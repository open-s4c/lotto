#lang typed/racket

(provide SUT-generate-random-transition-chain
         SUT-available-transitions
         SUT-take-transition
         new-SUTState
         SUTState-task-states
         SUTState-cur-task
         SUTTransition
         SUTTransition-action
         SUTTransition-tid
         SUTState
         RealTaskId
         RealTaskId?
         TaskState)

(require "automaton.rkt")

;; -----------------------------------------------------------------------------
;; SUT state
;; -----------------------------------------------------------------------------
(define-type TaskState (U 'ZERO 'CREATED 'RUNNING 'CAPTURED 'BLOCKED 'WAITING 'FINALIZING))
(define-type TaskStates (HashTable RealTaskId TaskState))
(define-type RealTaskId Positive-Integer)
(define-type SchedulableTaskId (U RealTaskId 'ANY_TASK))
(define-type TaskId (U SchedulableTaskId 'NO_TASK))

(define RealTaskId? positive-integer?)

(struct SUTState
        ([task-states : TaskStates] [cur-task : SchedulableTaskId] [last-created-task : RealTaskId])
  #:transparent
  #:mutable)

(: new-SUTState (-> SUTState))
(define (new-SUTState)
  (SUTState (make-hash (list (cons 1 'ZERO))) ;; state of task 1
            1 ;; current task
            1))

;; -----------------------------------------------------------------------------
;; SUT transitions
;; -----------------------------------------------------------------------------
(define-type Action
             (U 'CREATE 'INITIALIZE 'CAPTURE 'WAKE 'PICK 'BLOCK 'WAIT 'UNBLOCK 'FORK 'FINALIZE))
(struct SUTTransition ([action : Action] [tid : SchedulableTaskId]) #:transparent)

(: select-tasks-by-state (-> TaskStates (-> TaskState Boolean) (Listof RealTaskId)))
(define (select-tasks-by-state T p)
  (map (lambda ([tid-state : (Pairof RealTaskId TaskState)]) (car tid-state))
       (filter (lambda ([tid-state : (Pairof RealTaskId TaskState)]) (p (cdr tid-state)))
               (hash->list T))))

(: state-blocked? (-> TaskState Boolean))
(define (state-blocked? state)
  (match state
    ['BLOCKED #t]
    ['WAITING #t]
    [_ #f]))

(: select-blocked-tasks (-> TaskStates (Listof RealTaskId)))
(define (select-blocked-tasks T)
  (select-tasks-by-state T state-blocked?))

(: select-unblocked-tasks (-> TaskStates (Listof RealTaskId)))
(define (select-unblocked-tasks T)
  (select-tasks-by-state T (lambda ([s : TaskState]) (or (equal? s 'CAPTURED) (equal? s 'CREATED)))))

(: SUT-available-transitions (-> SUTState (Listof SUTTransition)))
(define/match (SUT-available-transitions S)
  [((SUTState T id _))
   (let* ([blocked-tasks (select-blocked-tasks T)]
          [state (hash-ref T id #f)]
          [unblock-transitions (map (curry SUTTransition 'UNBLOCK) blocked-tasks)]
          [unblocked-tasks (select-unblocked-tasks T)]
          [active-task-actions (append-map (lambda ([a : Action])
                                             (map (curry SUTTransition a)
                                                  (match a
                                                    ['CREATE (list id)]
                                                    ['INITIALIZE (list id)]
                                                    ['CAPTURE (list id)]
                                                    ['BLOCK (list id)]
                                                    ['WAIT (list id)]
                                                    ['FORK (list id)]
                                                    ['FINALIZE (list id)]
                                                    ['WAKE (cons 'ANY_TASK unblocked-tasks)]
                                                    ['PICK unblocked-tasks])))
                                           (match state
                                             ['ZERO '(CREATE)]
                                             ['CREATED '(INITIALIZE)]
                                             ['RUNNING '(CAPTURE BLOCK WAIT FORK FINALIZE)]
                                             ['CAPTURED '(WAKE)]
                                             ['BLOCKED '(WAKE)]
                                             ['WAITING '(WAKE)]
                                             ['FINALIZING '(WAKE)]
                                             [#f
                                              (assert (curry equal? 'ANY_TASK))
                                              '(PICK)]))])
     (append active-task-actions unblock-transitions))])

(: SUT-take-transition (-> SUTState SUTTransition Void))
(define/match (SUT-take-transition S t)
  [((SUTState T id l) (SUTTransition a task))
   (let ([state (hash-ref T id #f)])
     (match a
       ['PICK
        (assert id (curry equal? 'ANY_TASK))
        (set-SUTState-cur-task! S task)
        (with-asserts ([task RealTaskId?])
                      (assert (hash-ref T task) (curry equal? 'CAPTURED))
                      (hash-set! T task 'RUNNING))]
       ['WAKE
        (cond
          [(equal? state 'FINALIZING) (hash-remove! T id)])
        (set-SUTState-cur-task! S task)
        (cond
          [(RealTaskId? task)
           (assert (hash-ref T task) (curry equal? 'CAPTURED))
           (hash-set! T task 'RUNNING)])]
       ['UNBLOCK
        (with-asserts ([task RealTaskId?])
                      (assert (hash-ref T task) state-blocked?)
                      (hash-set! T task 'CAPTURED))]
       ['CREATE
        (assert id (curry equal? task))
        (with-asserts ([id RealTaskId?])
                      (assert state (curry equal? 'ZERO))
                      (hash-set! T id 'CREATED))]
       ['INITIALIZE
        (assert id (curry equal? task))
        (with-asserts ([id RealTaskId?])
                      (assert state (curry equal? 'CREATED))
                      (hash-set! T id 'CAPTURED))]
       ['CAPTURE
        (assert id (curry equal? task))
        (assert state (curry equal? 'RUNNING))
        (with-asserts ([id RealTaskId?]) (hash-set! T id 'CAPTURED))]
       ['BLOCK
        (assert id (curry equal? task))
        (assert state (curry equal? 'RUNNING))
        (with-asserts ([id RealTaskId?]) (hash-set! T id 'BLOCKED))]
       ['WAIT
        (assert id (curry equal? task))
        (assert state (curry equal? 'RUNNING))
        (with-asserts ([id RealTaskId?]) (hash-set! T id 'WAITING))]
       ['FORK
        (assert id (curry equal? task))
        (assert state (curry equal? 'RUNNING))
        (let ([new-task (+ 1 l)])
          (assert id RealTaskId?)
          (hash-set! T id 'CAPTURED)
          (set-SUTState-last-created-task! S new-task)
          (set-SUTState-cur-task! S new-task)
          (hash-set! T new-task 'ZERO))]
       ['FINALIZE
        (assert id (curry equal? task))
        (assert state (curry equal? 'RUNNING))
        (assert id RealTaskId?)
        (hash-set! T id 'FINALIZING)])
     (void))])

;; -----------------------------------------------------------------------------
;; SUT simulation
;; -----------------------------------------------------------------------------

(: SUT-generate-transition-chain
   (-> (-> SUTState (Listof SUTTransition) SUTTransition)
       SUTState
       Nonnegative-Integer
       (Values SUTState (Listof SUTTransition))))
(define (SUT-generate-transition-chain filter state length)
  (generate-transition-chain SUT-take-transition SUT-available-transitions filter state length))

(: SUT-generate-random-transition-chain
   (-> SUTState Nonnegative-Integer (Values SUTState (Listof SUTTransition))))
(define (SUT-generate-random-transition-chain state length)
  (generate-random-transition-chain SUT-take-transition SUT-available-transitions state length))
