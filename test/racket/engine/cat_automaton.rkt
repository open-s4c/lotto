#lang typed/racket

(provide Cat-available-transitions
         Cat-take-transition
         CatTransition
         CatState)

;; -----------------------------------------------------------------------------
;; cat state
;; -----------------------------------------------------------------------------
(define-type (CatState c) (U 'NONE c))

;; -----------------------------------------------------------------------------
;; cat transitions
;; -----------------------------------------------------------------------------
(define-type (CatTransition c) (U 'NONE c))

(: Cat-available-transitions (All (c) (-> (Listof c) (-> (CatState c) (Listof (CatTransition c))))))
(define (Cat-available-transitions cats)
  (match-lambda
    ['NONE (cons 'NONE cats)]
    [c (list 'NONE c)]))

(: Cat-take-transition (All (c) (-> (CatState c) (CatTransition c) (CatState c))))
(define (Cat-take-transition s t)
  (assert (or (equal? t 'NONE) (equal? s 'NONE) (equal? s t)))
  (match* (s t)
    [(_ 'NONE) s]
    [('NONE _) t]
    [(_ _) 'NONE]))
