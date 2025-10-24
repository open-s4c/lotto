#lang racket

(require rackunit)
(require ffi/unsafe)

;; -----------------------------------------------------------------------------
;; ctype definitions and cfunc registrations
;; -----------------------------------------------------------------------------

(require "mock/tidset.rkt")
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
  (many (lambda (k i) (call tidset_insert s k)) v))

(define (has-many s v)
  (many (lambda (k i) (check-true (call tidset_has s k))) v))

(define (get-many s v)
  (many (lambda (k i) (check-equal? (call tidset_get s i) k)) v))
(define (remove-many s v)
  (many (lambda (k i) (call tidset_remove s k)) v))

(define (not-has-many s v)
  (many (lambda (k i) (check-false (call tidset_has s k))) v))

(define (predicate v)
  (if (equal? 0 (remainder v 4)) #f #t))

(define NO_TASK 0)

;; -----------------------------------------------------------------------------
;; test cases
;; -----------------------------------------------------------------------------

;; allocate space for a tidset
(define vals '(1 2 3 4 5 10 34))
(define vals2 '(20 22 50 60 70 80 90))
(define nvals '(100 200 400))
(define odds '(1 3 5))

(test-case "initialize, insert and free"
  (define s (alloc-tidset))
  (call tidset_init s)
  (check-equal? 0 (call tidset_size s))
  (call tidset_insert s 1)
  (check-equal? 1 (call tidset_size s) "one item")
  (check-false (call tidset_has s 3) "item not in")
  (check-true (call tidset_has s 1) "item in")
  (call tidset_clear s)
  (check-equal? 0 (call tidset_size s) "empty again")
  (call tidset_fini s)
  ;; s was allocated with racket's malloc, no need to free.
  )

(test-case "insert, has, get many times"
  (define s (alloc-tidset))
  (call tidset_init s)
  (insert-many s vals)
  (has-many s vals)
  (check-equal? (length vals) (call tidset_size s))
  (get-many s vals)
  (call tidset_fini s))

(test-case "extend and duplicate items"
  (define s (alloc-tidset))
  (call tidset_init s)
  (insert-many s vals)
  (insert-many s vals2)
  (check-equal? (* 2 (length vals)) (call tidset_size s) "expected twice the length")
  (remove-many s vals)
  (check-equal? (length vals) (call tidset_size s) "old length")
  (call tidset_fini s))

(test-case "items not contained"
  (define s (alloc-tidset))
  (call tidset_init s)
  (insert-many s vals)
  (not-has-many s nvals)
  (call tidset_fini s))

(test-case "insert exist id"
  (define s (alloc-tidset))
  (call tidset_init s)
  (insert-many s vals)
  (call tidset_insert s 1)
  (check-equal? (length vals) (call tidset_size s) "existed id should not be insert")
  (call tidset_fini s))

(test-case "set new id and exist id"
  (define s (alloc-tidset))
  (call tidset_init s)
  (insert-many s vals)
  (check-equal? 3 (call tidset_get s 2))
  (call tidset_set s 2 6)
  (check-equal? 6 (call tidset_get s 2))
  (call tidset_set s 2 1)
  (check-equal? 1 (call tidset_get s 2))
  (check-equal? 6 (call tidset_get s 0))
  (call tidset_fini s))

(define tidset_cmp_gt (cfunc tidset_cmp_gt))
(define tidset_cmp_lt (cfunc tidset_cmp_lt))

(test-case "sorted items"
  (define s (alloc-tidset))
  (call tidset_init s)
  (insert-many s vals)
  (check-equal? (call tidset_get s 0) (first vals))
  (call tidset_sort s tidset_cmp_gt)
  (call tidset_sort s tidset_cmp_gt)
  (check-equal? (call tidset_get s 0) (first vals))
  (call tidset_sort s tidset_cmp_lt)
  (call tidset_sort s tidset_cmp_lt)
  (check-equal? (call tidset_get s 0) (last vals))
  (call tidset_sort s tidset_cmp_gt)
  (check-equal? (call tidset_get s 0) (first vals))
  (call tidset_fini s))

(test-case "clone of tidset"
  (define s (alloc-tidset))
  (call tidset_init s)
  (insert-many s vals)
  (define copy (alloc-tidset))
  (call tidset_init copy)
  (call tidset_sort s tidset_cmp_gt)
  (call tidset_copy copy s)
  (check-equal? (length vals) (call tidset_size copy))
  (check-equal? (length vals) (call tidset_size s))
  (has-many copy vals)
  (get-many copy vals)
  (check-true (call tidset_equals s copy))
  (call tidset_fini copy)
  (call tidset_fini s))

(test-case "corner cases and clear"
  (define s (alloc-tidset))
  (call tidset_init s)
  (insert-many s vals)
  (check-equal? NO_TASK (call tidset_get s 1000) "invalid value with get large index")
  (call tidset_clear s)
  (check-equal? 0 (call tidset_size s) "empty after clear")
  (call tidset_remove s 2)
  (check-equal? 0 (call tidset_size s))
  (check-equal? NO_TASK (call tidset_get s 0))
  (call tidset_fini s)
  (call tidset_fini s))

(test-case "intersect, contains_all"
  (define s (alloc-tidset))
  (call tidset_init s)
  (insert-many s vals)
  (define s_odd (alloc-tidset))
  (call tidset_init s_odd)
  (insert-many s_odd odds)
  (call tidset_intersect s s_odd)
  (check-true (call tidset_contains_all s s_odd)))

(test-case "subtract, filter, make_first"
  (define s (alloc-tidset))
  (call tidset_init s)
  (insert-many s vals)
  (define s_odd (alloc-tidset))
  (call tidset_init s_odd)
  (insert-many s_odd odds)
  (call tidset_subtract s s_odd)
  (check-false (call tidset_has s 1))
  (check-false (call tidset_has s 3))
  (check-false (call tidset_has s 5))
  (check-equal? 34 (call tidset_get s 0))
  (call tidset_make_first s 2)
  (check-equal? 2 (call tidset_get s 0))
  (call tidset_filter s predicate)
  (check-false (call tidset_has s 4)))

(test-case "msize, marshal, unmarshal"
  (define s (alloc-tidset))
  (call tidset_init s)
  (check-equal? 0 (call tidset_size s))
  (cast s _tidset-pointer _marshable-pointer)
  (check-equal? (- (ctype-sizeof _tidset_t) (ctype-sizeof _marshable_t)) (call tidset_msize s))
  (define buf (malloc (* 2 (ctype-sizeof _tidset_t))))
  (ptr-equal? (ptr-add buf 1 _tidset_t) (call tidset_marshal s buf))

  (define t (alloc-tidset))
  (call tidset_init t)
  (cast t _tidset-pointer _marshable-pointer)
  (define ret (call tidset_unmarshal t buf))
  (check-true (call tidset_equals s t)))
