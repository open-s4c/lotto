#lang racket

(require rackunit)
(require ffi/unsafe)

;; -----------------------------------------------------------------------------
;; ctype definitions and cfunc registrations
;; -----------------------------------------------------------------------------

(require "mock/trace_flat.rkt")
(load-lib "libbase_component")

;; -----------------------------------------------------------------------------
;; helper procedures
;; -----------------------------------------------------------------------------

(define-cstruct _trace_s
                ([append_f _trace_append] [next_f _trace_next]
                                          [advance_f _trace_advance]
                                          [last_f _trace_last]
                                          [forget_f _trace_forget]
                                          [clear_f _trace_clear]
                                          [load_f _trace_load]
                                          [save_f _trace_save]
                                          [save_to_f _trace_save_to]))

(define (alloc-trace_s)
  (cast (malloc (ctype-sizeof _trace_s)) _pointer _trace-pointer))

(define-cstruct _stream_s ([write_f _stream_write] [read_f _stream_read] [close_f _stream_close]))

(define (alloc-stream_s)
  (cast (malloc (ctype-sizeof _stream_s)) _pointer _stream_s-pointer))
;; -----------------------------------------------------------------------------
;; test cases
;; -----------------------------------------------------------------------------

(test-case "creat, append, last, advance"
  (define s (alloc-stream_s))
  (define tp (call trace_flat_create s))
  (define r (alloc-record_s))
  (call trace_append tp r)
  (check-equal? r (call trace_last tp))
  (call trace_advance tp)
  (check-false (call trace_last tp)))

(test-case "forget"
  (define s (alloc-stream_s))
  (define tp (call trace_flat_create s))
  (define r1 (alloc-record_s))
  (define r2 (alloc-record_s))
  (call trace_append tp r1)
  (call trace_append tp r2)
  (check-equal? r2 (call trace_last tp))
  (call trace_forget tp)
  (check-equal? r1 (call trace_last tp))
  (call trace_forget tp)
  (check-false (call trace_last tp)))

(test-case "clear"
  (define s (alloc-stream_s))
  (define tp (call trace_flat_create s))
  (define r1 (alloc-record_s))
  (define r2 (alloc-record_s))
  (call trace_append tp r1)
  (call trace_append tp r2)
  (call trace_clear tp)
  (check-false (call trace_last tp)))
