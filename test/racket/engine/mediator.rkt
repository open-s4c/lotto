#lang racket

(require ffi/unsafe
         ffi/unsafe/define)
(require rackunit)

(require dynamic-ffi/unsafe)
(require unix-signals)
(require racket/format)
(require racket/match)

;(require "../../test/unit/engine/base/base.rkt")

(define flags
  (list "-Lbuild/src/engine/base -lbase_testing "
        "-Lbuild/src/engine/real -lreal_bypass "
        "src/mediator/mediator.c "
        "-O0 -g "))

(println "loading mediator.c")
(define (this-dir fn)
  (build-path (path-only (path->complete-path (find-system-path 'run-file))) fn))

(define-inline-ffi mdi
                   #:compile-flags (apply string-append flags)
                   #:headers (list "include/lotto/mediator.h"
                                   "include/lotto/engine/engine.h"
                                   "include/lotto/engine/plan.h"
                                   "include/lotto/engine/category.h"
                                   "include/lotto/engine/context.h")
                   #:non-literal #t
                   ;;(file->string (this-dir "mediator_rkt.h"))
                   (file->string "src/mediator/mediator_rkt.h"))

;; basic types and constants
(define _size_t _uint64)
(define _task_id _uint64)
(define NO_TASK 0)
(define _null (cast '0 _uint64 _pointer))
(define _medistatus
  (_enum '(MEDIATOR_OK = 0 MEDIATOR_ABORT = 001 MEDIATOR_SHUTDOWN = 002 MEDIATOR_SNAPSHOT = 004)))

;; -----------------------------------------------------------------------------
;; helper functions
;; -----------------------------------------------------------------------------
(define (prefix-symbol prfx sym)
  (string->symbol (string-append prfx (symbol->string sym))))

(define (make-object t)
  (define size t)
  ;(define data (crypto-random-bytes size))
  (define data (malloc size))
  (define p (cast data _pointer (_cpointer t)))
  p)

(capture-signal! 'SIGABRT)
(define (aborted?)
  (sync/timeout (lambda () #f) next-signal-evt))

;; convert symbol into a category number
(define (category sym)
  (if (string-prefix? (symbol->string sym) "CAT_")
      sym
      (mdi (prefix-symbol "CAT_" sym))))

;; convert a sequence of action symbols into an integer
(define (gen-actions . syms)
  (define (get-sym sym)
    (if (string-prefix? (symbol->string sym) "ACTION_")
        (mdi sym)
        (mdi (prefix-symbol "ACTION_" sym))))
  (foldl + 0 (map get-sym syms)))

;; pick one of
(define (rnd-one-of l)
  (list-ref l (random 0 (length l))))
(define (with-prefix values prfx)
  (foldl (lambda (val prev)
           (if (string-prefix? (symbol->string val) prfx)
               (cons val prev)
               prev))
         '()
         values))

(println "DONE HERE")

(define (my-equal? a b)
  (check-equal? a b)
  (equal? a b))

(define (my-not-equal? a b)
  (check-not-equal? a b)
  (not (equal? a b)))

(define (my-true a)
  (check-true a)
  a)
(define (my-false a)
  (check-false a)
  (not a))

;;;;
;;;;(define (filter-by-id lst id)
;;;;    (filter pair? (for/list ([event lst])
;;;;        (match event
;;;;               [(list 'capture i cat act nxt) (if (equal? i id) event null)]
;;;;               [_ null] ))))
;;;;
;;;;;;(define lstx (for/list ([_ (in-range 0 10)]) (create)))
;;;;;;(filter-by-id lstx 0)

(println "----------------------------------------------------------------")
(define mediator (make-object (mdi 'mediator_t)))

(define (context-set! ctx id cat)
  (ptr-set! ctx _uint64 2 cat)
  (ptr-set! ctx _uint64 3 id)
  ctx)

(define (make-context id cat)
  (define ctx (make-object (mdi 'context_t)))
  (context-set! ctx id (mdi cat)))

(define (plan-set! plan actions next)
  (ptr-set! plan _uint64 0 actions)
  (ptr-set! plan _uint64 1 next)
  plan)

(define (make-plan actions next)
  (define plan (make-object (mdi 'plan_t)))
  (plan-set! plan actions next))

(define (plan-actions plan)
  (ptr-ref plan _uint64))

(define (get-val sym)
  (mdi sym))

(define (mediator-id mediator)
  (ptr-ref mediator _task_id))

(define (mediator-plan mediator)
  (ptr-add mediator 8))

(define (mediator-actions mediator)
  (ptr-ref mediator _uint64 1))

(define (mediator-next mediator)
  (ptr-ref mediator _uint64 2))

(define (mock-count sym)
  (mdi 'mock_count (mdi (prefix-symbol "CALLED_" sym))))

(define (mock-reset)
  (mdi 'mock_reset))

(define (mediator-set mediator id actions next)
  (ptr-set! mediator _uint64 0 id)
  (ptr-set! mediator _uint64 1 actions)
  (ptr-set! mediator _uint64 2 next))

(define (check-mediator-init mediator id ret)
  (and (equal? (mediator-id mediator) id)
       (check-true (< 0 (mock-count 'engine_resume)))
       (check-equal? ret (get-val 'MEDIATOR_OK))))
-----------------------------------------------------------
(define (test-mediator-init mediator id)
  (mock-reset)
  (let ([ret (mdi 'mediator_init mediator id _null)]) (check-mediator-init mediator id ret)))

(define (test-mediator-resume mediator id cat status)
  (mock-reset)
  (mdi 'expect_switcher_yield id)

  ;; prepare context
  (define ctx (make-context id (category cat)))

  (define acts (plan-actions (mediator-plan mediator)))
  (define should-yield (let ([wake (get-val 'ACTION_YIELD)]) (= (bitwise-and acts wake) wake)))

  ;; call
  (define ret (mdi 'mediator_resume mediator ctx))

  ;; check
  (and (check-false (aborted?))
       (if should-yield
           (check-true (= 1 (mock-count 'switcher_yield)))
           (check-true (= 0 (mock-count 'switcher_yield))))
       (check-true (= 1 (mock-count 'engine_resume)))
       (check-equal? ret (get-val status))))

(define (test-mediator-return mediator id cat)
  (mock-reset)
  ;; prepare context
  (define ctx (make-context id (category cat)))

  ;; call
  (mdi 'mediator_return mediator ctx)

  (and (check-false (aborted?)) (check-true (< 0 (mock-count 'engine_return)))))

(define (test-mediator-capture mediator id cat next actions)
  (mock-reset)

  ;; prepare context
  (define ctx (make-context id (category cat)))

  ;; prepare switcher_wake
  (mdi 'expect_switcher_wake next)

  ;; prepare engine_capture
  (define acts
    (if (number? actions)
        actions
        (apply gen-actions actions)))
  (define plan (make-plan acts next))
  (mdi 'mock_engine_capture plan)

  ;; call
  (define ret (mdi 'mediator_capture mediator ctx))

  (define should-wake (let ([wake (get-val 'ACTION_WAKE)]) (= (bitwise-and acts wake) wake)))

  ;;check
  (and (check-false (aborted?))
       (check-true (= 1 (mock-count 'engine_capture)))
       (if should-wake
           (check-true (= 1 (mock-count 'switcher_wake)))
           (check-true (= 0 (mock-count 'switcher_wake))))
       ;;(check-false ret)
       ))

(define (gen-call-sequence n)
  (define call-types
    '(init capture
           resume
           return))
  (for/list ([_ (range 0 n)])
    (rnd-one-of call-types)))

(define (gen-cat-sequence n)
  (define categories (with-prefix (hash-keys (mdi)) "CAT_"))
  (for/list ([_ (range 0 n)])
    (rnd-one-of categories)))

(define (gen-action-sequence n)
  (define max-actions (apply + (map (lambda (v) (mdi v)) (with-prefix (hash-keys (mdi)) "ACTION_"))))
  (for/list ([_ (range 0 n)])
    (random 0 max-actions)))

(define (gen-ids-sequence ids n)
  (for/list ([_ (range 0 n)])
    (rnd-one-of ids)))

(define (gen-sequence id ids n)
  (map list
       (gen-call-sequence n)
       (for/list ([_ (range 0 n)])
         id)
       (gen-cat-sequence n)
       (gen-action-sequence n)
       (gen-ids-sequence ids n)))

(define valid-actions
  (list (gen-actions 'CALL 'YIELD 'RESUME)
        (gen-actions 'WAKE 'YIELD 'RESUME)
        (gen-actions 'WAKE 'CALL 'RETURN 'YIELD 'RESUME)
        (gen-actions 'RETURN 'YIELD 'RESUME)
        (gen-actions 'YIELD 'RESUME)
        (gen-actions 'WAKE)
        (gen-actions 'RESUME)
        (gen-actions 'RESUME 'CONTINUE)))

(define (gen-valid-sequence id ids n)
  (define call-types
    '(init capture
           resume
           return))
  (define categories
    '(CAT_NONE CAT_CALL CAT_TASK_INIT CAT_TASK_BLOCK CAT_TASK_CREATE CAT_BEFORE_WRITE))
  ;(with-prefix (hash-keys (mdi)) "CAT_"))
  (define (iter id ids n seq)
    (if (= n 0)
        seq
        (let* ([ct (rnd-one-of call-types)]
               [cat (rnd-one-of categories)]
               [act (rnd-one-of valid-actions)]
               [nxt (rnd-one-of ids)]
               [ev (list ct id cat act nxt)]
               [nseq (append seq (list ev))])
          (printf "this? ~a\n" nseq)

          (if (is-valid nseq)
              (iter id ids (- n 1) nseq)
              (iter id ids n seq)))))
  (iter id ids n '()))

(define (event-ct ev)
  (list-ref ev 0))
(define (event-id ev)
  (list-ref ev 1))
(define (event-cat ev)
  (list-ref ev 2))
(define (event-act ev)
  (list-ref ev 3))
(define (event-nxt ev)
  (list-ref ev 4))

(define (actions-match? acts . actions)
  (my-equal? acts (apply gen-actions actions)))

(define (is-valid-resume pcat seq k)
  (or (null? seq)
      (let* ([ev (first seq)]
             [ct (event-ct ev)]
             [id (event-id ev)]
             [cat (event-cat ev)]
             [act (event-act ev)]
             [nxt (event-nxt ev)]
             [kk (+ k 1)])
        (and (my-equal? ct 'resume)
             (my-equal? pcat cat)
             ;;(actions-match? act 'YIELD 'RESUME)
             (is-valid-capture (rest seq) kk)))))

(define (is-valid-return pcat seq k)
  (or (null? seq)
      (let* ([ev (first seq)]
             [ct (event-ct ev)]
             [id (event-id ev)]
             [cat (event-cat ev)]
             [act (event-act ev)]
             [nxt (event-nxt ev)]
             [kk (+ k 1)])
        (and (my-equal? ct 'return)
             (my-equal? pcat cat)
             ;;(actions-match? act 'RETURN 'YIELD 'RESUME)
             (is-valid-resume cat (rest seq) kk)))))

(define (is-valid-capture seq k)
  (or (null? seq)
      (and (my-equal? (event-ct (first seq)) 'capture)
           (if (equal? (event-cat (first seq)) 'CAT_TASK_INIT)
               (= k 1)
               (> k 1))
           (not (equal? (event-cat (first seq)) 'CAT_NONE))
           (let* ([ev (first seq)]
                  [ct (event-ct ev)]
                  [id (event-id ev)]
                  [cat (event-cat ev)]
                  [act (event-act ev)]
                  [nxt (event-nxt ev)]
                  [kk (+ k 1)])
             (printf "~a\n" seq)
             (cond
               [(equal? cat 'CAT_TASK_CREATE)
                (and (actions-match? act 'CALL 'YIELD 'RESUME)
                     (my-equal? nxt id)
                     (is-valid-resume cat (rest seq) kk))]
               [(equal? cat 'CAT_TASK_INIT)
                (and (actions-match? act 'WAKE 'YIELD 'RESUME) (is-valid-resume cat (rest seq) kk))]
               [(equal? cat 'CAT_TASK_FINI)
                (and (actions-match? act 'WAKE) (my-not-equal? nxt id) (null? (rest seq)))]
               [(equal? cat 'CAT_TASK_BLOCK)
                (and (actions-match? act 'WAKE 'CALL 'RETURN 'YIELD 'RESUME)
                     (my-not-equal? nxt id)
                     (is-valid-return cat (rest seq) kk))]
               [(equal? cat 'CAT_CALL)
                (and (actions-match? act 'WAKE 'CALL 'RETURN 'YIELD 'RESUME)
                     (my-not-equal? nxt id)
                     (is-valid-return cat (rest seq) kk))]
               [else
                (or (and (actions-match? act 'WAKE 'YIELD 'RESUME)
                         (not (equal? nxt id))
                         (is-valid-resume cat (rest seq) kk))
                    (and (actions-match? act 'RESUME 'CONTINUE)
                         (not (equal? nxt id))
                         (is-valid-resume cat (rest seq) kk)))])))))

(define (is-valid-init event)
  (match event
    [(list 'init _ 'CAT_NONE _ _) #t]
    [_ (my-true #f)]))

(define (is-valid seq)
  (or (null? seq) (and (is-valid-init (first seq)) (is-valid-capture (rest seq) 1))))

(define (eval-valid-sequence seq)
  (if (null? seq)
      #t
      (begin
        (printf "===> test event ~a\n" (first seq))
        (and (match (first seq)
               [(list 'init id _ _ _) (test-mediator-init mediator id)]
               [(list 'capture id cat act nxt) (test-mediator-capture mediator id cat nxt act)]
               [(list 'resume id cat act nxt) (test-mediator-resume mediator id cat 'MEDIATOR_OK)]
               [(list 'return id cat act nxt) (test-mediator-return mediator id cat)])
             (eval-valid-sequence (rest seq))))))

(capture-signal! 'SIGABRT)
;;(define seq (gen-valid-sequence 1 '(1 2 3) 30))
(define seq
  '((init 1
          CAT_NONE
          0
          1)
    (capture 1 CAT_TASK_INIT 49 3)
    (resume 1 CAT_TASK_INIT 96 1)
    (capture 1 CAT_BEFORE_WRITE 96 2)
    (resume 1 CAT_BEFORE_WRITE 56 1)
    (capture 1 CAT_BEFORE_WRITE 49 3)))
;(eval-valid-sequence seq)
