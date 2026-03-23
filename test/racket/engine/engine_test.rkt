#lang racket

(require rackunit
         unix-signals
         ffi/unsafe)

(require "mock/engine.rkt")
(load-lib "libengine_component")

;; -----------------------------------------------------------------------------
;; support functions
;; -----------------------------------------------------------------------------

;;(call logger_set_level 'LOGGER_DEBUG)

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
  (foldl + 0 (for/list ([sym syms]) (get-sym sym))))

(define CAPTURE_BEFORE 5)
(define EVENT_TASK_INIT 170)
(define EVENT_TASK_FINI 171)
(define EVENT_TASK_CREATE 172)
(define EVENT_CALL 173)
(define EVENT_TASK_BLOCK 197)

(define-cstruct _capture_point
  ([src_chain _chain_id]
   [src_type _type_id]
   [blocking _uint8]
   [id _task_id]
   [vid _task_id]
   [pc _uintptr_t]
   [func _string]
   [func_addr _uintptr_t]
   [payload _pointer]
   [decision _pointer]))
(define (alloc-capture_point)
  (cast (malloc 'raw (ctype-sizeof _capture_point))
        _pointer
        _capture_point-pointer))

(define _engine_capture (_fun _capture_point-pointer -> _plan))
(define _engine_init (_fun _pointer _pointer -> _void))
(define _engine_fini (_fun _capture_point-pointer _reason -> _int))
(define _engine_resume (_fun _capture_point-pointer -> _void))
(define _engine_return (_fun _capture_point-pointer -> _void))
(define _sequencer_capture (_fun _capture_point-pointer -> _plan))

(define (cat->type cat)
  (match cat
    ['CAT_TASK_BLOCK EVENT_TASK_BLOCK]
    ['CAT_TASK_INIT EVENT_TASK_INIT]
    ['CAT_TASK_FINI EVENT_TASK_FINI]
    ['CAT_TASK_CREATE EVENT_TASK_CREATE]
    ['CAT_CALL EVENT_CALL]
    [_ 0]))

(define (new-cp id [cat 'CAT_NONE] [blocking #f])
  (let ([cp (alloc-capture_point)])
    (call sys_memset cp 0 (ctype-sizeof _capture_point))
    (set-capture_point-src_chain! cp (if (equal? cat 'CAT_NONE) 0 CAPTURE_BEFORE))
    (set-capture_point-src_type! cp (cat->type cat))
    (set-capture_point-blocking! cp (if blocking 1 0))
    (set-capture_point-id! cp id)
    (set-capture_point-func! cp "foo")
    cp))

(define ANY_TASK (cast -1 _int64 _task_id))
(define NULL (cast 0 _uint64 _pointer))

;; -----------------------------------------------------------------------------
;; corner cases
;; -----------------------------------------------------------------------------

(call engine_init NULL NULL)

;; STEP 1. engine starts by resuming a task
(call engine_resume (new-cp 1))

;; STEP 2. after that the task blocks
(mock sequencer_capture
      (let ([clk 0]
            [p (alloc-plan)])
        (lambda (cp)
          (set! clk (+ clk 1))
          (set-plan-clk! p clk)
          (set-plan-next! p ANY_TASK)
          (set-plan-actions! p (actions->int 'WAKE 'BLOCK 'RETURN 'YIELD 'RESUME))
          (ptr-ref p _plan))))
(void (call engine_capture (new-cp 1 'CAT_TASK_BLOCK #t)))

;; STEP 3. another task resumes
(call engine_resume (new-cp 2))

;; STEP 4. some task returns
(void (call engine_capture (new-cp 2 'CAT_TASK_BLOCK #t)))

;; STEP 5. let's finish the engine
(define r (call engine_fini (new-cp 1) 'REASON_SUCCESS))
(check-equal? r 0)
(check-false (aborted?))

;; STEP 6. another task returns;
;; we should not fail here. If return is called after fini, we should just ignore it.
(call engine_return (new-cp 2 'CAT_CALL #t))
(check-false (aborted?))
