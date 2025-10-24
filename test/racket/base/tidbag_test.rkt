#lang racket

(require rackunit)
(require ffi/unsafe)

;; -----------------------------------------------------------------------------
;; ctype definitions and cfunc registrations
;; -----------------------------------------------------------------------------

(require "mock/tidbag.rkt")
(load-lib "libbase_component")

;; -----------------------------------------------------------------------------
;; helper procedures
;; -----------------------------------------------------------------------------

(define (many f v)
  (define (many-iter v i)
    (cond
      [(> (length v) 0)
       (f (first v) i)
       (many-iter (rest v) (+ i 1))]))
  (many-iter v 0))

(define (insert-many s v)
  (many (lambda (k i) (call tidbag_insert s k)) v))

(define (has-many s v)
  (many (lambda (k i) (check-true (call tidbag_has s k))) v))

(define (get-many s v)
  (many (lambda (k i) (check-equal? (call tidbag_get s i) k)) v))
(define (remove-many s v)
  (many (lambda (k i) (call tidbag_remove s k)) v))

(define (not-has-many s v)
  (many (lambda (k i) (check-false (call tidbag_has s k))) v))

(define (predicate id)
  (if (even? id) #t #f))

(define NO_TASK 0)

;; -----------------------------------------------------------------------------
;; test cases
;; -----------------------------------------------------------------------------

;; allocate space for a tidbag
(define vals '(1 2 3 4 5 6 7 8))
(define nvals '(100 200 400))

(test-case "init, get, set, remove, has, clear, fini"
  (define s (alloc-tidbag))
  (call tidbag_init s)
  ;; tbag->size == 0
  (check-equal? NO_TASK (call tidbag_get s 1))
  (call tidbag_set s 1 1)
  (call tidbag_remove s 1)
  (check-false (call tidbag_has s 1))
  ;; tbag->size > 0
  (insert-many s nvals)
  (check-equal? 3 (call tidbag_size s))
  (check-equal? NO_TASK (call tidbag_get s 5))
  (check-equal? 100 (call tidbag_get s 0))
  (call tidbag_set s 0 150)
  (check-equal? 150 (call tidbag_get s 0))
  (check-true (call tidbag_has s 200))
  (call tidbag_remove s 200)
  (check-false (call tidbag_has s 200))
  ;; idx > tbag->size
  (check-equal? NO_TASK (call tidbag_get s 3))
  (call tidbag_set s 3 500)
  (check-false (call tidbag_has s 500))
  ;; clear, fini
  (call tidbag_clear s)
  (check-equal? 0 (call tidbag_size s))
  (call tidbag_fini s)
  (free s))

(test-case "expand, copy, filter, equals"
  (define s (alloc-tidbag))
  (call tidbag_init_cap s 5)
  ;; expand
  (insert-many s vals)
  (check-equal? 8 (call tidbag_size s))
  ;; copy
  (define t (alloc-tidbag))
  (call tidbag_init t)
  (call tidbag_copy t s)
  ;; equals
  (check-true (call tidbag_equals t s))
  ;; filter
  (call tidbag_filter s predicate)
  (check-false (call tidbag_has s 1))
  (call tidbag_fini t)
  (call tidbag_fini s)
  (free t)
  (free s))

(test-case "msize, marshal, unmarshal"
  (define s (alloc-tidbag))
  (call tidbag_init s)
  (cast s _tidbag-pointer _marshable-pointer)
  (define buf (malloc (* 2 (ctype-sizeof _tidbag_t))))
  (ptr-equal? (ptr-add buf 1 _tidbag_t) (call tidbag_marshal s buf))
  (define t (alloc-tidbag))
  (cast t _tidbag-pointer _marshable-pointer)
  (define alloc_size (marshable-alloc_size s))
  (set-marshable-alloc_size! t alloc_size)
  (call tidbag_unmarshal t buf)
  (check-true (call tidbag_equals t s))
  (free s))
