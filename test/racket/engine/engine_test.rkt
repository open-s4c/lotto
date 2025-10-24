#lang racket

(require rackunit
         unix-signals
         ffi/unsafe)

(require "mock/engine.rkt")
(load-lib "libengine_component")

;; -----------------------------------------------------------------------------
;; support functions
;; -----------------------------------------------------------------------------

;;(call logger_set_level 'LOG_DEBUG)

(void (capture-signal! 'SIGABRT))
(define (aborted?)
  (not (not (sync/timeout (lambda () #f) next-signal-evt))))

(define (prefix-symbol prfx sym)
  (string->symbol (string-append prfx (symbol->string sym))))

(define (actions->int . syms)
  (define (get-sym sym)
    (if (string-prefix? (symbol->string sym) "ACTION_")
        (cast sym _action _uint32)
        (cast (prefix-symbol "ACTION_" sym) _action _uint32)))
  (foldl + 0 (map get-sym syms)))

(define (new-context id [cat 'CAT_NONE])
  (let ([ctx (alloc-context)])
    (set-context-func! ctx "foo")
    (set-context-id! ctx id)
    (set-context-cat! ctx cat)
    ctx))

(define ANY_TASK (cast -1 _int64 _task_id))

;; -----------------------------------------------------------------------------
;; corner cases
;; -----------------------------------------------------------------------------

;; STEP 1. engine starts by resuming a task
(call engine_resume (new-context 1))

;; STEP 2. after that the task blocks
(mock sequencer_capture
      (let ([clk 0]
            [p (alloc-plan)])
        (lambda (ctx)
          (set! clk (+ clk 1))
          (set-plan-clk! p clk)
          (set-plan-next! p ANY_TASK)
          (set-plan-actions! p (actions->int 'WAKE 'BLOCK 'RETURN 'YIELD 'RESUME))
          p)))
(void (call engine_capture (new-context 1 'CAT_TASK_BLOCK)))

;; STEP 3. another task resumes
(call engine_resume (new-context 2))

;; STEP 4. some task returns
(call engine_return (new-context 1))

;; STEP 4. some task returns
(void (call engine_capture (new-context 2 'CAT_TASK_BLOCK)))

;; STEP 5. let's finish the engine
(define r (call engine_fini (new-context 1) 'REASON_SUCCESS))
(check-equal? r 0)
(check-false (aborted?))

;; STEP 6. another task returns;
;; we should not fail here. If return is called after fini, we should just ignore it.
(call engine_return (new-context 2))
(check-false (aborted?))
