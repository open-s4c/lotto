#lang racket

(require rackunit
         unix-signals
         ffi/unsafe
         syntax/parse/define)

(require (only-in "blocking-engine.rkt"
                  call
                  load-lib
                  alloc-context
                  _engine_init
                  _engine_resume
                  _engine_capture
                  _engine_return
                  set-context-func!
                  set-context-id!
                  set-context-cat!
                  _task_id
                  _plan
                  plan-next
                  _engine_fini))
(require (only-in "recorder.rkt"
                  _stream_file_out
                  _stream_file_in
                  _trace_flat_create
                  _stream_file_alloc
                  _trace_save
                  _trace_load
                  _stream_close))
(require "sut_cat_automaton.rkt")
(require "sut_automaton.rkt")
(require "automaton.rkt")

(capture-signal! 'SIGABRT)
(define (aborted?)
  (not (equal? #f (sync/timeout (lambda () #f) next-signal-evt))))

;; add a procedure to be called at exit
(define (at-exit f)
  (plumber-add-flush! (current-plumber) ;
                      (lambda (_) (f)))
  (void))

;; configure random seed
(define seed (current-seconds)) ; to set the seed manually, replace (current-seconds)
(random-seed seed)
(at-exit (lambda () (printf "Initial seed: ~a\n" seed)))

;; -----------------------------------------------------------------------------
;; ffi helpers
;; -----------------------------------------------------------------------------

(define (make-context cat id)
  (let ([ctx (alloc-context)])
    (set-context-id! ctx id)
    (set-context-cat! ctx cat)
    (set-context-func! ctx "func")
    ctx))

(define ANY_TASK (cast -1 _int64 _task_id))

(define (new-context id [cat 'CAT_NONE])
  (let ([ctx (alloc-context)])
    (set-context-func! ctx "foo")
    (set-context-id! ctx id)
    (set-context-cat! ctx cat)
    ctx))

(define NULL (cast 0 _uint64 _pointer))

(define cats '((CAPTURE . CAT_BEFORE_WRITE) (CAPTURE . CAT_BEFORE_READ)))

;; -----------------------------------------------------------------------------
;; transition processing
;; -----------------------------------------------------------------------------

(define capture-actions '(CAPTURE INITIALIZE FORK BLOCK FINALIZE))
(define capture-transition? (compose (curryr member capture-actions) SUTTransition-action car))
(define next #f)
(define id-cats '())

(define (matches-next-task? t)
  (or (not next)
      (match t
        [(cons (SUTTransition 'WAKE 'ANY_TASK) _) (equal? next ANY_TASK)]
        [(cons (SUTTransition 'WAKE tid) _) (equal? next tid)]
        [_ #t])))

(define/match (matches-next-cat? t)
  [((cons _ cat)) (or (empty? id-cats) (not (capture-transition? t)) (equal? (cdar id-cats) cat))])

(define (matches-not-wait? t)
  (match t
    [(cons (SUTTransition 'WAIT _) _) #f]
    [_ #t]))

(define-syntax-rule (capture cat id)
  (begin
    (cond
      [(not (equal? id-cats '()))
       (check-equal? id (caar id-cats))
       (set! id-cats (cdr id-cats))])
    (set! next (plan-next (call engine_capture (make-context cat id))))
    (check-false (aborted?))))

(define-syntax resume
  (syntax-rules ()
    [(resume cat)
     (begin
       (check-not-equal? next #f)
       (cond
         [(not (equal? next ANY_TASK))
          (call engine_resume (make-context cat next))
          (check-false (aborted?))])
       (set! next #f))]
    [(resume cat id)
     (begin
       (call engine_resume (make-context cat id))
       (check-false (aborted?)))]))

(define-syntax-rule (return id)
  (begin
    (call engine_return (make-context 'CAT_CALL id))
    (check-false (aborted?))))

(define (simulate)
  ((SUTCat-generate-transition-chain cats)
   (lambda (_ ts)
     (let ([t (rnd-one-of (filter (conjoin matches-next-task? matches-next-cat? matches-not-wait?)
                                  ts))])
       (match t
         [(cons (SUTTransition 'WAKE _) cat) (resume cat)]
         [(cons (SUTTransition 'CAPTURE id) cat) (capture cat id)]
         [(cons (SUTTransition 'CREATE id) cat) (resume cat id)]
         [(cons (SUTTransition 'INITIALIZE id) cat) (capture cat id)]
         [(cons (SUTTransition 'PICK id) cat) (resume cat id)]
         [(cons (SUTTransition 'FORK id) cat) (capture cat id)]
         [(cons (SUTTransition 'BLOCK id) cat) (capture cat id)]
         [(cons (SUTTransition 'FINALIZE id) cat) (capture cat id)]
         [(cons (SUTTransition 'UNBLOCK id) cat) (return id)])
       t))
   (new-SUTCatState)
   1000))

;; -----------------------------------------------------------------------------
;; record
;; -----------------------------------------------------------------------------

(define cust (make-custodian))
(load-lib "libreplay_component" #:custodian cust)

(define trace-fn "replayer_test.trace")
(define trace-s (call stream_file_alloc))
(call stream_file_out trace-s trace-fn)
(define trace (call trace_flat_create trace-s))
(void (call engine_init NULL trace))

(define-values (s c) (simulate))

(void (call engine_fini (make-context 'CAT_NONE 1) 'REASON_SUCCESS))
(call trace_save trace)
(call stream_close trace-s)

;; -----------------------------------------------------------------------------
;; replay
;; -----------------------------------------------------------------------------

(custodian-shutdown-all cust)
(load-lib "libreplay_component" #:custodian cust)
(call stream_file_in trace-s trace-fn)
(set! trace (call trace_flat_create trace-s))
(call trace_load trace)
(void (call engine_init trace NULL))

(set! id-cats
      (map (match-lambda
             [(cons (SUTTransition _ tid) cat) (cons tid cat)])
           (filter capture-transition? c)))

(call-with-values simulate void)

(void (call engine_fini (make-context 'CAT_NONE 1) 'REASON_SUCCESS))
