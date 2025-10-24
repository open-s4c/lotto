#lang racket

(require rackunit
         unix-signals
         ffi/unsafe)

(require "mock/sequencer.rkt")
(load-lib "libmocked_sequencer_component")

(capture-signal! 'SIGABRT)
(define (aborted?)
  (not (equal? #f (sync/timeout (lambda () #f) next-signal-evt))))

;(define cfg (alloc-sequencer_cfg))

;(call sequencer_init cfg)

(define NO_TASK 0)
(define NULL (cast 0 _uint64 _pointer))

;; -----------------------------------------------------------------------------
;; handler
;;
;; The handler component represents the whole chain of handlers connected
;; at the dispatcher.
;; -----------------------------------------------------------------------------
;;(struct handler ([tidset #:mutable] [seed #:mutable]) #:transparent)
;;
;;(define hs (handler '() 0))
;;
;;(define (reset-handler)
;;  (set! hs (handler '() 0)))
;;
;;(define (handle ctx)
;;  (cond
;;    [(empty? (handler-tidset hs)) NO_TASK]
;;    [else (set-handler-seed! hs (+ 1 (handler-seed hs)))]))
;;
;; -----------------------------------------------------------------------------
;; recorder
;;
;; Mocks the recorder component. Perhaps we could use the real one, however.
;; -----------------------------------------------------------------------------

;;(struct record (clk state) #:transparent)
;;(define trace '())
;;
;;(mock recorder_record
;;      (lambda (clk) ;
;;        (let ([r (list (record clk hs))]) ;
;;          (printf "clk: ~a\n" clk)
;;          (set! trace (append trace r)))))
;;
;;(define (recorder-consume clk)
;;  (cond
;;    [(empty? trace) #f]
;;    [(= clk (record-clk (first trace)))
;;     (set! hs (record-state (first trace)))
;;     (set! trace (rest trace))
;;     (recorder-consume clk)]
;;    ;; clock should never be greater than record clk
;;    ;; assert that (< clk (record-clk (first trace)))
;;    [else #t]))
;;(mock recorder_replay recorder-consume)
;;;; -----------------------------------------------------------------------------
;;;; event sequence generator
;;;; -----------------------------------------------------------------------------
;;
;;(struct seqgen
;;        ([task-states #:mutable] ;; hash map tid -> task-state
;;         [sys-state #:mutable] ;; OK or FINAL
;;         [cur-task #:mutable] ;; some tid
;;         [seq #:mutable]) ;; (events ...)
;;  #:transparent)
;;
;;(define (new-seqgen)
;;  (seqgen (hash 1 'ZERO) ;; state of task 1
;;          'OK ;; state of system
;;          1 ;; current task
;;          '() ;; sequence of events
;;          ))
;;
;;;; create a sequence (returns the generator) with length at most 'len'
;;(define (generate-event-sequence len)
;;  (define gen (new-seqgen))
;;  (define (iter gen len)
;;    (if (zero? len)
;;        gen
;;        (match (seqgen-sys-state gen)
;;          ['FINAL gen]
;;          ['OK (iter (task-transition gen) (- len 1))])))
;;  (iter gen len))
;;
;;;; -----------------------------------------------------------------------------
;;;; scheduler: picks a task filtered with a function
;;;; -----------------------------------------------------------------------------
;;
;;(struct seq-plan (actions next) #:transparent)
;;
;;(define (rnd-one-of l)
;;  (list-ref l (random 0 (length l))))
;;
;;(define (pick-task filter-fun gen)
;;  (define keys (hash-keys (seqgen-task-states gen)))
;;  (define filtered-keys (filter filter-fun (cons 'ANY_TASK keys)))
;;  (if (empty? filtered-keys) 'NO_TASK (rnd-one-of filtered-keys)))
;;
;;(define (get-last-id seq id)
;;  (if (empty? seq)
;;      id
;;      (let ([ev (first seq)])
;;        (if (equal? (car ev) id) ;
;;            (get-last-id (rest seq) id)
;;            (car ev)))))
;;
;;;; return plan pairs (actions next-id)
;;(define (seq-plan-step id cat gen)
;;  (define any-id (nonblocked-id (seqgen-task-states gen)))
;;  (define except-id
;;    (lambda (tid)
;;      (and (not (equal? id tid)) ;
;;           (any-id tid))))
;;  (match cat
;;    ['CAT_TASK_CREATE (seq-plan '(CALL YIELD RESUME) id)]
;;    ['CAT_TASK_INIT (seq-plan '(WAKE YIELD RESUME) (get-last-id (seqgen-seq gen) id))]
;;    ['CAT_TASK_FINI (seq-plan '(WAKE) (pick-task except-id gen))]
;;    ['CAT_TASK_BLOCK (seq-plan '(WAKE CALL RETURN YIELD RESUME) (pick-task except-id gen))]
;;    ['CAT_CALL (seq-plan '(WAKE CALL RETURN YIELD RESUME) (pick-task except-id gen))]
;;    [_ (seq-plan '(WAKE YIELD RESUME) (pick-task any-id gen))]
;;    ;;;;    ;;[_ (seq-plan '(RESUME CONTINUE) id)]
;;    ;;;;    [_ (seq-plan '(RESUME ABORT) id)]
;;    ;;;;    ;;[_ (seq-plan '(RESUME SHUTDOWN) id)]
;;    ))
;;(define (project-seq seq id)
;;  (filter (lambda (ev) (equal? id (car ev))) seq))
;;
;;(define (last-cat-of id seq)
;;  (let ([ev (car (project-seq seq id))]) (third ev)))
;;
;;(define (prefix-symbol prfx sym)
;;  (string->symbol (string-append prfx (symbol->string sym))))
;;
;;(define (gen-action . syms)
;;  (define (get-sym sym)
;;    (if (string-prefix? (symbol->string sym) "ACTION_")
;;        (cast sym _action _uint32)
;;        (cast (prefix-symbol "ACTION_" sym) _action _uint32)))
;;  (foldl + 0 (map get-sym syms)))
;;
;;;; -----------------------------------------------------------------------------
;;;; preform the state transition for the current task (seqgen-cur-task)
;;;; -----------------------------------------------------------------------------
;;(define (nonblocked-id T)
;;  (lambda (tid)
;;    (or (equal? 'ANY_TASK tid) ;
;;        (not (equal? 'BLOCKED (hash-ref T tid))))))
;;
;;(define (nonblocked-real-id T)
;;  (lambda (tid)
;;    (and (not (equal? 'ANY_TASK tid)) ;
;;         (not (equal? 'BLOCKED (hash-ref T tid))))))
;;
;;;(define (nonblocked-id T)
;;;  (lambda (tid)
;;;    (and (not (equal? 'ANY_TASK tid)) ;
;;;         (not (equal? 'BLOCKED (hash-ref T tid))))))
;;
;;(define (blocked-id T)
;;  (lambda (tid) (equal? 'BLOCKED (hash-ref T tid))))
;;
;;(define (unblock-some G)
;;  (define T (seqgen-task-states G))
;;  (define K (filter (blocked-id T) (hash-keys T)))
;;  (for ([id K])
;;    (if (equal? (random 2) 1) ;
;;        (set! T (hash-set T id 'UNBLOCKED))
;;        '()))
;;  (set-seqgen-task-states! G T))
;;
;;(define (get-next-task G)
;;  (unblock-some G)
;;  (if (not (equal? (seqgen-cur-task G) 'ANY_TASK))
;;      (seqgen-cur-task G)
;;      (let* ([T (seqgen-task-states G)] ;
;;             [id (pick-task (nonblocked-real-id T) G)])
;;        (if (equal? id 'NO_TASK) (get-next-task G) id))))
;;
;;(define (print-gen G)
;;  (printf "----------------------\n")
;;  (printf "States:\n")
;;  (for ([i (hash-keys (seqgen-task-states G))])
;;    (printf "\t~a => ~a \n" i (hash-ref (seqgen-task-states G) i)))
;;  (printf "Events:\n")
;;  (for ([i (reverse (seqgen-seq G))])
;;    (printf "\t~a\n" i)))
;;
;;(define (check-gen G msg)
;;  (for ([id (hash-keys (seqgen-task-states G))])
;;    (if (equal? id 'ANY_TASK)
;;        (begin
;;          (printf "ERROR ~a\n" msg)
;;          (print-gen G)
;;          (error "Inconsistent"))
;;        '())))
;;
;;(define (task-transition G)
;;  (check-gen G "START")
;;  (define id (get-next-task G))
;;  (define S (seqgen-sys-state G))
;;  (define T (seqgen-task-states G))
;;  (check-gen G "END")
;;  (define except-id (lambda (tid) (not (equal? id tid))))
;;  (match (hash-ref T id)
;;    ['ZERO
;;     (let* ([step `(,id resume CAT_NONE _)]
;;            [nseq (cons step (seqgen-seq G))]
;;            [nT (hash-set T id 'CREATED)])
;;       (seqgen nT S id nseq))]
;;    ['CREATED
;;     (let* ([cat 'CAT_TASK_INIT]
;;            [plan (seq-plan-step id cat G)]
;;            [step (list id 'capture cat plan)]
;;            [nseq (cons step (seqgen-seq G))]
;;            [nT (hash-set T id 'INITIALIZING)])
;;       (seqgen nT S (seq-plan-next plan) nseq))]
;;    ['INITIALIZING
;;     (let* ([step `(,id resume CAT_TASK_INIT _)]
;;            [nseq (cons step (seqgen-seq G))]
;;            [nT (hash-set T id 'RUNNING)])
;;       (seqgen nT S id nseq))]
;;    ['RUNNING
;;     (let* ([cat (rnd-one-of '(CAT_BEFORE_WRITE CAT_CALL CAT_BEFORE_READ CAT_TASK_CREATE))]
;;            [plan (seq-plan-step id cat G)]
;;            [step (list id 'capture cat plan)]
;;            [nseq (cons step (seqgen-seq G))] ;
;;            [nT (hash-set T id 'CAPTURED)]
;;            [next (seq-plan-next plan)])
;;
;;       ;; update task states if necessary
;;       (match step
;;         [(list _ _ 'CAT_TASK_CREATE _)
;;          (begin ;; create new task and initialize state
;;            (set! next (+ 1 (length (hash-keys (seqgen-task-states G)))))
;;            (set! nT (hash-set nT next 'ZERO)))]
;;         ;;[(list _ _ 'CAT_TASK_BLOCK _) (set! nT (hash-set nT id 'BLOCKED))]
;;         [(list _ _ 'CAT_CALL _)
;;          (set! nT (hash-set nT id 'BLOCKED))
;;          (set! next (pick-task (nonblocked-id nT) G))]
;;         [_ '()])
;;       ;; if blocking, consider with slack
;;       (seqgen nT S next nseq))]
;;    ['CAPTURED
;;     (let* ([seq (seqgen-seq G)]
;;            [step (list id 'resume (last-cat-of id seq) '_)] ;
;;            [nseq (cons step seq)] ;
;;            [nT (hash-set T id 'RUNNING)])
;;       (seqgen nT S id nseq))]
;;    ['BLOCKED
;;     (begin
;;       (printf "ERROR: task ~a is ~a (BLOCKED)\n" id (hash-ref (seqgen-task-states G) id))
;;       (printf "States:\n")
;;       (for ([i (hash-keys (seqgen-task-states G))])
;;         (printf "\t~a => ~a ~a\n" i (hash-ref T i) (hash-ref (seqgen-task-states G) i)))
;;       (for ([i (reverse (seqgen-seq G))])
;;         (printf "\t~a\n" i))
;;       ;(printf "~a" (seqgen-cur-task G))
;;       (error "should not schedule BLOCKED task"))]
;;
;;    ['UNBLOCKED
;;     (let* ([seq (seqgen-seq G)]
;;            [step (list id 'return (last-cat-of id seq) '_)] ;
;;            [nseq (cons step seq)] ;
;;            [nT (hash-set T id 'CAPTURED)])
;;       (seqgen nT S id nseq))]
;;    ['FINALIZING
;;     (let* ([step `(,id resume CAT_BEFORE_WRITE _)] ;
;;            [nseq (cons step (seqgen-seq G))]
;;            [nT (hash-set T id 'FINAL)])
;;       (seqgen nT S id nseq))]))
;;
;;;; -----------------------------------------------------------------------------
;;;; Perform tests agains the C interface
;;;; -----------------------------------------------------------------------------
;;
;;(define g (generate-event-sequence 100))
;;(printf "Event sequence\n")
;;(for ([i (reverse (seqgen-seq g))])
;;  (printf "\t~a\n" i))
;;(printf "\n")
;;(printf "Task states : ~a\n" (seqgen-task-states g))
;;(printf "Running task: ~a\n" (seqgen-cur-task g))
;;(printf "System state: ~a\n" (seqgen-sys-state g))
;;
;;(for ([i (reverse (seqgen-seq g))])
;;  (printf "===> ~a\n" i)
;;  (match i
;;    [(list id 'resume cat _)
;;     (begin
;;       (mock sequencer_resume
;;             (lambda (ctx)
;;               ;;     (printf "RESUME ~a\n" (context-id ctx))
;;               (check-equal? (context-id ctx) id)))
;;
;;       (define ctx (alloc-context))
;;       (set-context-id! ctx id)
;;       (set-context-cat! ctx cat)
;;       (call engine_resume ctx)
;;
;;       (check-false (aborted?)))]
;;
;;    [(list id 'capture cat splan)
;;     (let* ([_nxt (seq-plan-next splan)]
;;            [next (if (equal? _nxt 'ANY_TASK) (cast -1 _int64 _task_id) _nxt)]
;;            [actions (apply gen-action (seq-plan-actions splan))])
;;       (mock sequencer_capture
;;             (lambda (ctx clk)
;;               (check-equal? (context-id ctx) id)
;;               (check-equal? (context-cat ctx) cat)
;;
;;               (define plan (alloc-plan))
;;               (set-plan-next! plan next)
;;               (set-plan-actions! plan actions)
;;               ;;         (printf "CAPTURE ~a\n" (context-id ctx))
;;               plan))
;;
;;       (define ctx (alloc-context))
;;       (set-context-id! ctx id)
;;       (set-context-cat! ctx cat)
;;       (define p (call engine_capture ctx))
;;
;;       (check-false (aborted?)))]
;;
;;    [(list id 'return cat _)
;;     (begin
;;       (mock sequencer_return
;;             (lambda (ctx)
;;               ;;       (printf "RETURN ~a\n" (context-id ctx))
;;               (check-equal? (context-id ctx) id)))
;;
;;       (define ctx (alloc-context))
;;       (set-context-id! ctx id)
;;       (set-context-cat! ctx cat)
;;       (call engine_return ctx)
;;
;;       (check-false (aborted?)))]))
