#lang typed/racket

(provide ResourceId
         ResourceAction
         DeadlockType
         new-SUTCatResState
         SUTCatResState
         SUTCatResState-sutcat
         SUTCatResState-resources
         SUTCatResState-deadlock
         SUTCatResTransition
         SUTCatRes-generate-transition-chain)

(require "automaton.rkt")
(require "cat_automaton.rkt")
(require "sut_automaton.rkt")
(require "sut_cat_automaton.rkt")

(define-type DeadlockType (U 'DT_NONE 'DT_DEADLOCK 'DT_EXTRA_UNLOCK 'DT_LOST_LOCK))
(define-type ResourceId Nonnegative-Integer)
(struct SUTCatResState
        ([sutcat : SUTCatState] [num-resources : Nonnegative-Integer]
                                [resources : (HashTable ResourceId (Listof RealTaskId))]
                                [deadlock : DeadlockType]
                                [target : DeadlockType])
  #:transparent
  #:mutable)
(define-type ResourceAction
             (U 'RA_NONE 'RA_ACQUIRE 'RA_RELEASE)) ;; NOTE: think about more complex state machine
(struct SUTCatResTransition ([tr : SUTCatTransition] [rid : ResourceId] [raction : ResourceAction])
  #:transparent
  #:mutable)

;; Function to build the wait-for graph
(: build-wait-for-graph
   (-> (HashTable ResourceId (Listof RealTaskId)) (HashTable RealTaskId (Listof RealTaskId))))
(define (build-wait-for-graph resources)
  (let ([graph
         :
         (HashTable RealTaskId (Listof RealTaskId))
         (make-hash)])
    (for-each (lambda ([entry : (Pairof ResourceId (Listof RealTaskId))])
                (match entry
                  [(cons _ (cons holder waiting-tasks))
                   (for-each (lambda ([waiting-task : RealTaskId])
                               ;; Add an edge from waiting task to holder
                               (hash-update! graph
                                             waiting-task
                                             (lambda ([existing : (Listof RealTaskId)])
                                               (cons holder existing))
                                             (lambda () (cast '() (Listof RealTaskId)))))
                             waiting-tasks)]))
              (hash->list resources))
    ;;  (printf "Graph: ~a\n" graph)
    graph))

;; Function to detect cycles in the wait-for graph
(: has-cycle? (-> (HashTable RealTaskId (Listof RealTaskId)) Boolean))
(define (has-cycle? graph)
  (let* ([visited (make-hash)]
         [rec-stack (make-hash)])
    (: dfs (-> RealTaskId Boolean))
    (define/match (dfs node)
      [(_)
       #:when (hash-ref rec-stack node #f)
       #t] ; Cycle detected
      [(_)
       #:when (hash-ref visited node #f)
       #f] ; Already visited
      [(_)
       (hash-set! visited node #t)
       (hash-set! rec-stack node #t)
       (if (ormap dfs (hash-ref graph node (lambda () (cast '() (Listof RealTaskId)))))
           #t
           (begin
             (hash-remove! rec-stack node)
             #f))])
    (ormap dfs (hash-keys graph))))

(: detect-deadlock? (-> SUTCatResState Boolean))
(define (detect-deadlock? sutcat-res-state)
  (has-cycle? (build-wait-for-graph (SUTCatResState-resources sutcat-res-state))))

(: new-SUTCatResState (-> Nonnegative-Integer DeadlockType SUTCatResState))
(define (new-SUTCatResState num target)
  (SUTCatResState (new-SUTCatState) num (make-hash) 'DT_NONE target))

(define cats
  '((CAPTURE . CAT_RSRC_ACQUIRING) (CAPTURE . CAT_RSRC_RELEASED) (WAIT . CAT_RSRC_ACQUIRING)))

(: SUTCatRes-available-transitions (-> SUTCatResState (Listof SUTCatResTransition)))
(define/match (SUTCatRes-available-transitions state)
  [((SUTCatResState s num ht _ target))
   (append-map
    (lambda ([t : SUTCatTransition])
      (match t
        [(cons (SUTTransition 'CAPTURE _) 'CAT_RSRC_ACQUIRING)
         (build-list num
                     (lambda ([i : Nonnegative-Integer])
                       (SUTCatResTransition t (+ i 1) 'RA_ACQUIRE)))]
        [(cons (SUTTransition 'WAIT _) 'CAT_RSRC_ACQUIRING)
         #:when (eq? target 'DT_DEADLOCK)
         (build-list num
                     (lambda ([i : Nonnegative-Integer])
                       (SUTCatResTransition t (+ i 1) 'RA_ACQUIRE)))]
        [(cons (SUTTransition 'WAIT task-id) 'CAT_RSRC_ACQUIRING)
         (apply
          append
          (build-list
           num
           (lambda ([i : Nonnegative-Integer])
             ;; Filter-out transitions that will result in a deadlock if target is not DT_DEADLOCK
             (let* ([rid (+ i 1)]
                    [tr (SUTCatResTransition t rid 'RA_ACQUIRE)]
                    [newht (hash-copy ht)]
                    [newstate (struct-copy SUTCatResState state [resources newht])]
                    [task-list (hash-ref newht rid (lambda () '()))]
                    [_ (assert task-id RealTaskId?)]
                    [updated-task-list (append task-list (list task-id))])
               (hash-set! newht rid updated-task-list)
               (if (detect-deadlock? newstate)
                   '()
                   (list tr))))))]
        [(cons _ 'CAT_RSRC_RELEASED)
         (build-list num
                     (lambda ([i : Nonnegative-Integer])
                       (SUTCatResTransition t (+ i 1) 'RA_RELEASE)))]
        [(cons (SUTTransition (or 'BLOCK 'UNBLOCK) _) _) '()]
        [_ (list (SUTCatResTransition t 0 'RA_NONE))]))
    ((SUTCat-available-transitions cats) s))])

(: SUTCatRes-take-transition (-> SUTCatResState SUTCatResTransition Void))
(define/match (SUTCatRes-take-transition s t)
  [((SUTCatResState (cons ss sc) _ ht _ _) (SUTCatResTransition (cons ts tc) rid raction))
   (SUT-take-transition ss ts)
   (match* (tc (SUTTransition-tid ts))
     [('NONE _) (void)]
     [('CAT_TASK_FINI id)
      (hash-remove! sc id)
      (when (ormap (lambda ([item : (Pairof ResourceId (Listof RealTaskId))])
                     (match item
                       [(cons _ value) (member id value)]))
                   (hash->list ht))
        (set-SUTCatResState-deadlock! s 'DT_LOST_LOCK))]
     [(_ id)
      (assert id RealTaskId?)
      (hash-set! sc id (Cat-take-transition (hash-ref sc id (lambda () 'CAT_NONE)) tc))])
   (match* (raction (SUTTransition-action ts)
                    (SUTTransition-tid ts)
                    (RealTaskId? (SUTTransition-tid ts))
                    (hash-ref ht rid (lambda () '())))
     ;; TODO: support recursive locks
     [('RA_ACQUIRE 'WAIT task-id #t task-list)
      (let* ([_ (assert (not (member task-id task-list)))]
             [updated-task-list (append task-list (list task-id))])
        (hash-set! ht rid updated-task-list)
        (when (detect-deadlock? s)
          (set-SUTCatResState-deadlock! s 'DT_DEADLOCK)))]
     [('RA_ACQUIRE 'CAPTURE task-id #t task-list)
      (let* (;; Here we try to acquire the same resource twice
             [_ (assert (not (member task-id task-list)))]
             [updated-task-list (append task-list (list task-id))])
        (hash-set! ht rid updated-task-list))]
     [('RA_RELEASE 'CAPTURE task-id #t task-list)
      ;; Here we try to release a resource that has not been aquired by a current task
      ;; Set the deadlock status DT_EXTRA_UNLOCK in a state
      #:when (or (empty? task-list) (not (member task-id task-list)))
      (set-SUTCatResState-deadlock! s 'DT_EXTRA_UNLOCK)]
     [('RA_RELEASE 'CAPTURE task-id #t task-list)
      (let* ([_ (assert (equal? (car task-list) task-id))]
             [updated-task-list (cdr task-list)])
        (if (null? updated-task-list)
            (hash-remove! ht rid)
            (begin
              (hash-set! ht rid updated-task-list)
              ;; Don't forget:
              ;; (1) Pick first task in the updated-task-list
              (let ([first-task (car updated-task-list)])
                ;; (2) If this task is not found in any other items in resource hash table, change its state to CAPTURED
                (unless (ormap (lambda ([item : (Pairof ResourceId (Listof RealTaskId))])
                                 (member first-task (cdr (cdr item))))
                               (hash->list ht))
                  (hash-set! (SUTState-task-states ss) first-task 'CAPTURED))))))]
     [(_ _ _ _ _) (void)])])

;; -----------------------------------------------------------------------------
;; SUTCatRes selectors
;; -----------------------------------------------------------------------------
(: selector (-> SUTCatResState (Listof SUTCatResTransition) SUTCatResTransition))
(define (selector state ts)
  (match state
    [(SUTCatResState (cons ss _) _ ht 'DT_NONE target)
     (rnd-one-of
      (filter (match-lambda
                ;; Exclude, if we try to WAKE but resource is not owned
                [(SUTCatResTransition (cons (SUTTransition 'WAKE tid) _) rid 'RA_ACQUIRE)
                 (member tid (hash-ref ht rid (lambda () '())))]
                ;; Exclude, if action is WAIT but we are the first one to acquire the resource
                ;; This transition must be nonblocking
                [(SUTCatResTransition (cons (SUTTransition 'WAIT tid) _) rid 'RA_ACQUIRE)
                 (let* ([task-list (hash-ref ht rid (lambda () '()))])
                   (and (not (empty? task-list)) (not (equal? tid (car task-list)))))]
                ;; Exclude, if action is CAPTURE, and we are in the list of tasks for a given resource
                [(SUTCatResTransition (cons (SUTTransition 'CAPTURE _) _) rid 'RA_ACQUIRE)
                 (empty? (hash-ref ht rid (lambda () '())))]
                ;; Exclude, if we try to release resource that is not acquired
                ;; or allow if target is DT_EXTRA_UNLOCK
                [(SUTCatResTransition (cons (SUTTransition 'CAPTURE tid) _) rid 'RA_RELEASE)
                 (or (member tid (hash-ref ht rid (lambda () '()))) (eq? 'DT_EXTRA_UNLOCK target))]
                [(SUTCatResTransition (cons (SUTTransition 'FINALIZE tid) _) _ _)
                 #:when (eq? target 'DT_LOST_LOCK)
                 (ormap (lambda ([item : (Pairof ResourceId (Listof RealTaskId))])
                          (match item
                            [(cons _ value) (member tid value)]))
                        (hash->list ht))]
                ;; Exclude, if we try to finalize task that owns/tries to own a resource
                [(SUTCatResTransition (cons (SUTTransition 'FINALIZE tid) _) _ _)
                 (and (not (ormap (lambda ([item : (Pairof ResourceId (Listof RealTaskId))])
                                    (match item
                                      [(cons _ value) (member tid value)]))
                                  (hash->list ht)))
                      (not (eq? (hash-count (SUTState-task-states ss)) 1)))]
                [_ #t])
              ts))]
    ;; If we triggered a deadlock condition, we must force stop generation of further transitions
    [_
     (begin
       (let ([ht (SUTState-task-states (car (SUTCatResState-sutcat state)))])
         (for-each (lambda ([item : (Pairof RealTaskId TaskState)])
                     (hash-set! ht (car item) 'WAITING))
                   (hash->list ht)))
       (SUTCatResTransition (cons (SUTTransition 'WAKE 'ANY_TASK) 'NONE) 0 'RA_NONE))]))

;; -----------------------------------------------------------------------------
;; SUTCatRes simulation
;; -----------------------------------------------------------------------------
(: SUTCatRes-generate-transition-chain
   (-> SUTCatResState Nonnegative-Integer (Values SUTCatResState (Listof SUTCatResTransition))))
(define (SUTCatRes-generate-transition-chain state length)
  (let-values ([(s t) (generate-transition-chain SUTCatRes-take-transition
                                                 SUTCatRes-available-transitions
                                                 selector
                                                 state
                                                 length)])
    (assert (equal? (SUTCatResState-deadlock state) (SUTCatResState-target state)))
    (values s t)))
