#lang racket

(require quickcheck
         rackunit
         rackunit/quickcheck)

(require "mock/recorder.rkt")
(load-lib "libmocked_recorder")

;; -----------------------------------------------------------------------------
;; suporting definitions
;; -----------------------------------------------------------------------------
(require ffi/unsafe
         unix-signals)

(define NO_TASK 0)
(define NULL (cast 0 _uint64 _pointer))

;; randomly pick one of a list
(define (one-of lst)
  (list-ref lst (random 0 (length lst))))

;; randomly executes one of the procedures in (either a b c)
;; They have  uniform probability of being chosen.
(define-syntax-rule (either last-trans trans ...)
  (let* ([lst (list trans ...)]
         [cnt (+ 1 (length lst))])
    (cond
      [(= 0 (random cnt)) trans] ...
      [else last-trans])))

;; add a procedure to be called at exit
(define (at-exit f)
  (plumber-add-flush! (current-plumber) ;
                      (lambda (_) (f)))
  (void))

;; configure random seed
(define seed (current-seconds))
;; manually set seed here to reproduce aborted runs
;;(define seed 1700759448)
(random-seed seed)
(at-exit (lambda () (printf "Initial seed: ~a\n" seed)))

;; replacement of ASSERT
(capture-signal! 'SIGABRT)
(define (aborted?)
  (not (not (sync/timeout (lambda () #f) next-signal-evt))))

;; disable verbose logging
;;(call logger_set_level 'LOG_ERROR)

;; -----------------------------------------------------------------------------
;; Generator
;; -----------------------------------------------------------------------------

;; number of tasks
(define N 4)
;; max clock gap
(define max-gap 5)
;; generator type
(struct gen (clk) #:mutable #:transparent)

;; update state and create arguments for recorder_record
(define (next g id clk)
  (let ([ctx (alloc-context)]
        [cat (one-of _category-domain)])
    (set-context-id! ctx id)
    (set-context-cat! ctx cat)
    (set-gen-clk! g clk)
    (cons ctx clk)))

;; step defines a successful step, ie,
;; - the id used to record is between 1 and N
;; - the clk used to record is great than the current clk
;;   but the clk may jump a bit
(define (step g)
  (let ([id (random 1 N)] ;
        [clk (+ (gen-clk g) (random 1 max-gap))]) ;
    (next g id clk)))

;; fail defines a failure step (it should be detected), s.t.,
;; - either the id 0
;; - or the clk has stopped
;; - or goes backwards
(define (fail-id g)
  (let ([id 0]
        [clk (+ 1 (gen-clk g))]) ;
    (next g id clk)))
(define (fail-clk-stop g)
  (let ([id (random 1 N)] ;
        [clk (gen-clk g)])
    (next g id clk)))
(define (fail-clk-back g)
  (let* ([id (random 1 N)] ;
         [gap (random 1 max-gap)]
         [cur-clk (gen-clk g)]
         [clk (if (> gap cur-clk)
                  0
                  (- cur-clk gap))])
    (next g id clk)))
(define (fail g)
  (either (fail-id) (fail-clk-stop) (fail-clk-back)))

;; -----------------------------------------------------------------------------
;; record and replay
;; -----------------------------------------------------------------------------

;; record calls the recorder with the given arguments and returns the
;; current task id in the context.
(define (record ctx clk)
  (call recorder_record ctx clk)
  (context-id ctx))

;; replay calls the recorder with the given arguments and returns the
;; current task id given by the replay.
(define (replay clk)
  (let ([ry (call recorder_replay clk)]) ;
    (if (equal? (replay-status ry) 'REPLAY_LOAD)
        (replay-id ry)
        '())))

;; -----------------------------------------------------------------------------
;; simple tests
;; -----------------------------------------------------------------------------

(test-case "what is replayed was recorded"
  ;; create in-memory trace
  (define trace (call trace_flat_create (alloc-stream_s)))
  ;; initialize recorder to record only
  (call recorder_init NULL trace)

  ;; perform record calls
  (define g (gen 0))
  (define recorded
    (for/list ([_ (range 1 5)])
      (let ([nxt (step g)]) (record (car nxt) (cdr nxt)))))
  ;(displayln recorded)

  ;; configure recorder to use trace as input
  (call recorder_init trace NULL)

  ;; perform replay calls;
  ;; The maximum clock during record was g-clk.
  ;; During replay, we call replay for every single clk
  (define max-clk (+ 1 (gen-clk g)))
  (define replayed
    ;; only at clks where there are records, the replay procedure returns an id.
    ;; Otherwise it returns an empty list. We flatten the result to filter the
    ;; empty lists out.
    (flatten (for/list ([clk (range 0 max-clk)])
               (replay clk))))
  ;(displayln replayed)

  (check-equal? recorded replayed)
  (check-false (aborted?)))

;; -----------------------------------------------------------------------------
;; record invariants
;; -----------------------------------------------------------------------------
(define (check-record-when fail-mode)
  (define trace (call trace_flat_create (alloc-stream_s)))
  (call recorder_init NULL trace)
  (define g (gen 0))
  (define max-calls 10)
  (define fail-at (random 1 max-calls))
  (for ([c (range 1 max-calls)])
    (let ([nxt (if (= c fail-at) ;
                   (fail-mode g)
                   (step g))])
      (record (car nxt) (cdr nxt))))
  (check-true (aborted?)))

(test-case "never record from id 0"
  (check-record-when fail-id))

(test-case "never record the same clock twice"
  (check-record-when fail-clk-stop))

(test-case "never record with clock in the past"
  (check-record-when fail-clk-back))

(test-case "no record after fini"
  (define trace (call trace_flat_create (alloc-stream_s)))
  (call recorder_init NULL trace)
  (define g (gen 0))
  (define fail-at (random 1 10))
  (for ([c (range 1 fail-at)])
    (let ([nxt (step g)]) (record (car nxt) (cdr nxt))))
  (call recorder_fini fail-at 0 'REASON_UNKNOWN)
  (step g) ; consume one clk
  (for ([c (range 1 5)])
    (let ([nxt (step g)]) (record (car nxt) (cdr nxt))))
  (check-true (aborted?)))

(test-case "no fini after fini"
  (define trace (call trace_flat_create (alloc-stream_s)))
  (call recorder_init NULL trace)
  (define g (gen 0))
  (for ([c (range 1 3)])
    (let ([nxt (step g)]) (record (car nxt) (cdr nxt))))
  (call recorder_fini 3 0 'REASON_UNKNOWN)
  (call recorder_fini 4 0 'REASON_UNKNOWN)
  (check-true (aborted?)))

;; -----------------------------------------------------------------------------
;; replay invariants
;; -----------------------------------------------------------------------------

;; create a trace file
(define trace-fn "recorder_test.trace")
(begin
  (define st (call stream_file_out trace-fn))
  (define trace (call trace_flat_create st))
  (call recorder_init NULL trace)
  (define g (gen 0))
  (define recorded
    (for/list ([_ (range 1 5)])
      (let ([nxt (step g)]) (record (car nxt) (cdr nxt)))))
  (call trace_save trace)
  (call stream_close st)
  (void))

(define (replay-check check)
  (define st (call stream_file_in trace-fn))
  (define trace (call trace_flat_create st))
  (call trace_load trace)
  (define max-clk (record_s-clk (call trace_last trace)))
  (call recorder_init trace NULL)
  (check trace max-clk))

(test-case "replay starts at zero"
  (replay-check (lambda (trace max-clk)
                  (call recorder_replay (random 1 max-clk))
                  (check-true (aborted?)))))

(test-case "replay clk contiguous"
  (replay-check (lambda (trace max-clk)
                  (call recorder_replay 0)
                  (call recorder_replay 1)
                  (check-true (> max-clk 2))
                  (call recorder_replay (random 3 max-clk))
                  (check-true (aborted?)))))

(test-case "replay clk never backwards"
  (replay-check (lambda (trace max-clk)
                  (for ([i (range 0 (- max-clk 1))])
                    (call recorder_replay i))
                  (call recorder_replay (random 0 (- max-clk 1)))
                  (check-true (aborted?)))))

(test-case "no replay after fini"
  (replay-check (lambda (trace max-clk)
                  (for ([i (range 0 3)])
                    (call recorder_replay i))
                  (call recorder_fini 2 1 'REASON_ABORT)
                  (call recorder_replay 3)
                  (check-true (aborted?)))))

(test-case "no replay at fini"
  (replay-check (lambda (trace max-clk)
                  (for ([i (range 0 3)])
                    (call recorder_replay i))
                  (call recorder_fini 3 1 'REASON_ABORT)
                  (call recorder_replay 3)
                  (check-true (aborted?)))))

(test-case "record always after or at replay"
  (replay-check (lambda (trace max-clk)
                  (for ([i (range 0 10)])
                    (call recorder_replay i))

                  ;; create a record in clk in 1..10
                  (define p (next (gen 0) 1 (random 1 10)))
                  (check-true (>= (cdr p) 1))
                  (check-true (< (cdr p) 10))
                  (call recorder_record (car p) (cdr p))
                  (check-true (aborted?)))))
