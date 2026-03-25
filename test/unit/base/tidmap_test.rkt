#lang racket

(require rackunit)
(require ffi/unsafe)

;; -----------------------------------------------------------------------------
;; ctype definitions and cfunc registrations
;; -----------------------------------------------------------------------------

(require "mock/tidmap.rkt")
(load-lib "libbase_component")

;; -----------------------------------------------------------------------------
;; helper procedures
;; -----------------------------------------------------------------------------

;;(call logger_set_level 'LOGGER_DEBUG)

(define (many f v)
  (define (many-iter v)
    (cond
      [(> v 0)
       (f v)
       (many-iter (- v 1))]))
  (many-iter v))

(define (register-many s v)
  (many (lambda (k) (call tidmap_register s k)) v))

;; -----------------------------------------------------------------------------
;; test cases
;; -----------------------------------------------------------------------------

(test-case "initialize, register, deregister, size, clear"
  (define mi (alloc-marshable))
  (set-marshable-alloc_size! mi (ctype-sizeof _tiditem_t))
  (define s (alloc-map))
  (call tidmap_init s mi)
  (check-equal? (call tidmap_size s) 0)
  (call tidmap_register s 1)
  (call tidmap_register s 2)
  (check-equal? (call tidmap_size s) 2)
  (call tidmap_deregister s 1)
  (check-equal? (call tidmap_size s) 1)
  (call tidmap_clear s)
  (check-equal? (call tidmap_size s) 0)
  (free s)
  (free mi))

(test-case "find, iterate, next"
  (define mi (alloc-marshable))
  (set-marshable-alloc_size! mi (ctype-sizeof _tiditem_t))
  (define s (alloc-map))
  (call tidmap_init s mi)
  (register-many s 4)
  (define it3 (call tidmap_find s 3))
  (check-equal? (mapitem-key it3) 3)
  (define it_n (call tidmap_next it3))
  (check-equal? (mapitem-key it_n) 4)
  (define it1 (call tidmap_iterate s))
  (check-equal? (mapitem-key it1) 1)
  (free s)
  (free mi))
