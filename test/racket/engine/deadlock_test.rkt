#lang racket

(require rackunit
         unix-signals
         ffi/unsafe)

(require "mock/engine.rkt")
(require "sut_automaton.rkt")
(require "automaton.rkt")
(require "resource.rkt")

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
;; Interesting seeds for 2 resources: 1, 9, 15, 20, 29 (the best example), 1731605125
;; (define seed 1732888729)

(random-seed seed)
(at-exit (lambda () (printf "Initial seed: ~a\n" seed)))

;; -----------------------------------------------------------------------------
;; ffi helpers
;; -----------------------------------------------------------------------------
(define NULL (cast 0 _uint64 _pointer))

(define (make-arg width value)
  (let* ([arg (alloc-arg)]
         [value-field (arg-value arg)])
    (set-arg-width! arg width)
    (union-set! value-field
                (match width
                  [(or 'ARG_U8 'ARG_EMPTY) 0]
                  ['ARG_U16 1]
                  ['ARG_U32 2]
                  ['ARG_U64 3]
                  ['ARG_U128 4]
                  ['ARG_PTR 5])
                value)
    (set-arg-value! arg value-field)
    arg))

(define-syntax-rule (make-context cat id args ...)
  (let* ([arg-list (list args ...)])
    (check-false (> (length arg-list) 4))
    (let* ([ctx (alloc-context)])
      (set-context-id! ctx id)
      (set-context-cat! ctx cat)
      (set-context-func! ctx "func")
      (set-context-args! ctx
                         (for/vector ([i (in-range 4)])
                           (if (< i (length arg-list))
                               (let ([arg (list-ref arg-list i)]) (make-arg (car arg) (cdr arg)))
                               (make-arg 'ARG_EMPTY 0))))
      ctx)))

;; -----------------------------------------------------------------------------
;; Perform tests against the C interface
;; -----------------------------------------------------------------------------
(define generation-target (string->symbol (car (vector->list (current-command-line-arguments)))))
(printf "Target condition is ~a\n" generation-target)
(define-values (state transitions)
  (SUTCatRes-generate-transition-chain (new-SUTCatResState 10 generation-target) 1000))
;; (printf "Event sequence\n")
;; (for ([tr transitions])
;;   (printf "\t~a\n" tr))
;; (printf "\n")
(printf "Task states : ~a\n" (SUTState-task-states (car (SUTCatResState-sutcat state))))
(printf "Running task: ~a\n" (SUTState-cur-task (car (SUTCatResState-sutcat state))))
(printf "Initial seed: ~a\n" seed)
(printf "Overall state: ~a\n" state)

(define cust (make-custodian))
(load-lib "libdeadlock_component" #:custodian cust)

(define _ps_subscribe (_fun _topic_t _pointer _pointer -> _void))
;; The first argument is actualy a structure passed by value, temporaritly specify a pointer type here
(define _ps_subscribe_f (_fun _pointer _pointer -> _void))

(define-syntax-rule (capture cat id args ...)
  (begin
    (call engine_capture (make-context cat id args ...))
    (check-false (aborted?))))

(define-syntax-rule (resume cat id)
  (cond
    [(not (equal? id 'ANY_TASK))
     (call engine_resume (make-context cat id))
     (check-false (aborted?))]))

(printf "\n=== Simulation start ===\n\n")

(define deadlock-detected #f)

(define (deadlock-cb u1 u2)
  (set! deadlock-detected #t))

(call ps_subscribe 'TOPIC_DEADLOCK_DETECTED (cast deadlock-cb _ps_subscribe_f _pointer) NULL)

(void (call engine_init NULL NULL))

(for ([tr transitions])
  #:break deadlock-detected
  (printf "===> ~a\n" tr)
  (match tr
    [(SUTCatResTransition (cons (SUTTransition 'WAKE id) cat) _ _) (resume cat id)]
    [(SUTCatResTransition (cons (SUTTransition (or 'CAPTURE 'WAIT) id) cat)
                          rid
                          (or 'RA_ACQUIRE 'RA_RELEASE))
     (capture cat id (cons 'ARG_PTR rid))]
    [(SUTCatResTransition (cons (SUTTransition 'CAPTURE id) cat) _ _) (capture cat id)]
    [(SUTCatResTransition (cons (SUTTransition 'CREATE id) cat) _ _) (resume cat id)]
    [(SUTCatResTransition (cons (SUTTransition 'INITIALIZE id) cat) _ _) (capture cat id)]
    [(SUTCatResTransition (cons (SUTTransition 'PICK id) cat) _ _) (resume cat id)]
    [(SUTCatResTransition (cons (SUTTransition 'FORK id) cat) _ _) (capture cat id)]
    [(SUTCatResTransition (cons (SUTTransition 'FINALIZE id) cat) _ _) (capture cat id)]))

(check-true deadlock-detected "Test must detect a deadlock")
