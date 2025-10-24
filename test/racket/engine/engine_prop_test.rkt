#lang racket

(require rackunit
         unix-signals
         ffi/unsafe)

(require "mock/engine.rkt")
(require "sut_automaton.rkt")
(require "sut_cat_automaton.rkt")
(require "automaton.rkt")
(load-lib "libengine_component")

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

(define (prefix-symbol prfx sym)
  (string->symbol (string-append prfx (symbol->string sym))))

(define (gen-action . syms)
  (define (get-sym sym)
    (if (string-prefix? (symbol->string sym) "ACTION_")
        (cast sym _action _uint32)
        (cast (prefix-symbol "ACTION_" sym) _action _uint32)))
  (foldl + 0 (map get-sym syms)))

(define ANY_TASK (cast -1 _int64 _task_id))

;; return actions
(define (make-plan cat prev next)
  (apply gen-action
         (match cat
           ['CAT_TASK_CREATE '(CALL YIELD RESUME)]
           ['CAT_TASK_INIT '(WAKE YIELD RESUME)]
           ['CAT_TASK_FINI '(WAKE)]
           ['CAT_TASK_BLOCK '(WAKE CALL RETURN YIELD RESUME)]
           ['CAT_CALL '(WAKE CALL RETURN YIELD RESUME)]
           [_ '(WAKE YIELD RESUME)])))

(define (make-context cat id)
  (let ([ctx (alloc-context)])
    (set-context-id! ctx id)
    (set-context-cat! ctx cat)
    (set-context-func! ctx "func")
    ctx))

(define cats '((CAPTURE . CAT_BEFORE_WRITE) (CAPTURE . CAT_BEFORE_READ)))

;; -----------------------------------------------------------------------------
;; mocks
;; -----------------------------------------------------------------------------

(define param-id (make-parameter 'INVALID))
(define param-clk (make-parameter 'INVALID))
(define param-cat (make-parameter 'INVALID))
(define param-loop-cont (make-parameter 'INVALID))

(mock sequencer_capture
      (lambda (ctx)
        (check-equal? (context-id ctx) (param-id))
        (check-equal? (context-cat ctx) (param-cat))
        (let* ([next-id (call/cc (lambda (cont) ((param-loop-cont) cont)))]
               [actions (make-plan (context-cat ctx) (context-id ctx) next-id)]
               [plan (alloc-plan)])
          (set-plan-next! plan next-id)
          (set-plan-actions! plan actions)
          (set-plan-clk! plan (param-clk))
          (set-plan-with_slack! plan false)
          (printf "[t:~a, clk:~a] CAPTURE ~a\n" (context-id ctx) (plan-clk plan) (context-cat ctx))
          plan)))

(mock sequencer_resume
      (lambda (ctx)
        (printf "[t:~a] RESUME ~a\n" (context-id ctx) (context-cat ctx))
        (check-equal? (context-id ctx) (param-id))))

(mock sequencer_return
      (lambda (ctx)
        (printf "[t:~a] RETURN ~a\n" (context-id ctx) (context-cat ctx))
        (check-equal? (context-id ctx) (param-id))))

;; -----------------------------------------------------------------------------
;; Perform tests agains the C interface
;; -----------------------------------------------------------------------------
(define-values (s c)
  ((SUTCat-generate-transition-chain cats)
   (match-lambda**
     [((cons (SUTState _ 'ANY_TASK _) _) ts) (rnd-one-of ts)]
     [((cons (SUTState T id _) _) ts)
      (rnd-one-of (if (equal? 'BLOCKED (hash-ref T id))
                      (filter (match-lambda
                                [(cons (SUTTransition 'UNBLOCK tid) _)
                                 #:when (equal? id tid)
                                 #f]
                                [(cons (SUTTransition 'WAIT _) _) #f]
                                [_ #t])
                              ts)
                      (filter (match-lambda
                                [(cons (SUTTransition 'WAIT _) _) #f]
                                [_ #t])
                              ts)))])
   (new-SUTCatState)
   1000))
(printf "Event sequence\n")
(for ([i c])
  (printf "\t~a\n" i))
(printf "\n")
(printf "Task states : ~a\n" (SUTState-task-states (car s)))
(printf "Running task: ~a\n" (SUTState-cur-task (car s)))

(let ([clock 0]
      [loop-cont #f]
      [capture-cont #f])
  (let-syntax ([capture (syntax-rules ()
                          [(capture cat id)
                           (begin
                             (set! clock (+ 1 clock))
                             (set! capture-cont
                                   (call/cc (lambda (cont)
                                              (parameterize ([param-clk clock]
                                                             [param-cat cat]
                                                             [param-id id]
                                                             [param-loop-cont cont])
                                                (call engine_capture (make-context cat id))
                                                #f))))
                             (cond
                               [(not capture-cont)
                                (check-false (aborted?))
                                (loop-cont)]))]
                          [(capture cat id next)
                           (begin
                             (set! clock (+ 1 clock))
                             (let ([capture-cont (call/cc (lambda (cont)
                                                            (parameterize ([param-clk clock]
                                                                           [param-cat cat]
                                                                           [param-id id]
                                                                           [param-loop-cont cont])
                                                              (call engine_capture
                                                                    (make-context cat id))
                                                              #f)))])
                               (cond
                                 [(not capture-cont) (check-false (aborted?))]
                                 [else (capture-cont next)])))])]
               [wake (syntax-rules ()
                       [(wake id)
                        (call/cc (lambda (cont)
                                   (set! loop-cont cont)
                                   (capture-cont id)))])]
               [resume (syntax-rules ()
                         [(resume cat id)
                          (begin
                            (parameterize ([param-id id])
                              (call engine_resume (make-context cat id)))
                            (check-false (aborted?)))])]
               [return (syntax-rules ()
                         [(return id)
                          (begin
                            (parameterize ([param-id id])
                              (call engine_return (make-context 'CAT_CALL id)))
                            (check-false (aborted?)))])])
    (for ([i c])
      (printf "===> ~a\n" i)
      (match i
        [(cons (SUTTransition 'WAKE 'ANY_TASK) _) (wake ANY_TASK)]
        [(cons (SUTTransition 'CAPTURE id) cat) (capture cat id)]
        [(cons (SUTTransition 'WAKE id) cat)
         (wake id)
         (resume cat id)]
        [(cons (SUTTransition 'CREATE id) cat) (resume cat id)]
        [(cons (SUTTransition 'INITIALIZE id) cat) (capture cat id)]
        [(cons (SUTTransition 'PICK id) cat) (resume cat id)]
        [(cons (SUTTransition 'FORK id) cat) (capture cat id ANY_TASK)]
        [(cons (SUTTransition 'BLOCK id) cat) (capture cat id)]
        [(cons (SUTTransition 'FINALIZE id) cat) (capture cat id)]
        [(cons (SUTTransition 'UNBLOCK id) cat) (return id)]))))
