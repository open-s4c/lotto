#lang racket

(require rackunit)
(require ffi/unsafe)

;; -----------------------------------------------------------------------------
;; ctype definitions and cfunc registrations
;; -----------------------------------------------------------------------------

(require "mock/bag.rkt")
(load-lib "libbase_component")

;; -----------------------------------------------------------------------------
;; helper procedures
;; -----------------------------------------------------------------------------

;;(call logger_set_level 'LOGGER_DEBUG)

;; -----------------------------------------------------------------------------
;; test cases
;; -----------------------------------------------------------------------------

(test-case "initialize, add, remove, size, clear, copy"
  (define mi (alloc-marshable))
  (set-marshable-alloc_size! mi (ctype-sizeof _bagitem))
  (define s (alloc-bag))
  (call bag_init s mi)
  (check-equal? (call bag_size s) 0)
  (call bag_add s)
  (call bag_add s)
  (check-equal? (call bag_size s) 2)
  (define dst (alloc-bag))
  (call bag_init dst mi)
  (call bag_copy dst s)
  (check-equal? (call bag_size s) (call bag_size dst))
  (let ([it (call bag_iterate s)]) ;
    (call bag_remove s it))
  (check-equal? (call bag_size s) 1)
  (call bag_clear s)
  (check-equal? (call bag_size s) 0)
  (call bag_clear s)
  (call bag_clear dst)
  (free s)
  (free dst)
  (free mi))
