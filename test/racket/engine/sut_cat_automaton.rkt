#lang typed/racket

(provide new-SUTCatState
         SUTCatState
         SUTCatTransition
         SUTCat-available-transitions
         SUTCat-generate-transition-chain)

(require "sut_automaton.rkt")
(require "cat_automaton.rkt")
(require "automaton.rkt")

;; -----------------------------------------------------------------------------
;; SUTCat state
;; -----------------------------------------------------------------------------
(define-type SUTCatState (Pairof SUTState (HashTable RealTaskId (CatState Symbol))))
(define-type Category Symbol)
(define-type Action Symbol)

(: new-SUTCatState (-> SUTCatState))
(define (new-SUTCatState)
  (let ([s
         :
         SUTState
         (new-SUTState)]
        [cs
         :
         (HashTable RealTaskId (CatState Symbol))
         (make-hash (list (cons 1 'CAT_NONE)))])
    (cons s cs)))

;; -----------------------------------------------------------------------------
;; SUTCat transitions
;; -----------------------------------------------------------------------------
(define-type SUTCatTransition (Pairof SUTTransition (CatTransition Symbol)))

(: SUTCat-available-transitions
   (-> (Listof (Pairof Action Category)) (-> SUTCatState (Listof SUTCatTransition))))
(define (SUTCat-available-transitions cats)
  (match-lambda
    [(cons s c)
     (append-map (lambda ([t : SUTTransition])
                   (let ([default (match t
                                    [(SUTTransition c 'ANY_TASK) '(NONE)]
                                    [(SUTTransition 'CREATE _) '(CAT_NONE)]
                                    [(SUTTransition 'INITIALIZE _) '(CAT_TASK_INIT)]
                                    [(SUTTransition 'CAPTURE _) '()]
                                    [(SUTTransition 'WAKE id)
                                     (list (let ([cat (hash-ref c id)])
                                             (assert cat symbol?)
                                             cat))]
                                    [(SUTTransition 'PICK id)
                                     (list (let ([cat (hash-ref c id)])
                                             (assert cat symbol?)
                                             cat))]
                                    [(SUTTransition 'BLOCK _) '(CAT_CALL)]
                                    [(SUTTransition 'WAIT _) '()]
                                    [(SUTTransition 'UNBLOCK _) '(NONE)]
                                    [(SUTTransition 'FORK _) '(CAT_TASK_CREATE)]
                                    [(SUTTransition 'FINALIZE _) '(CAT_TASK_FINI)])]
                         [custom (map (lambda ([pair : (Pairof Action Category)]) (cdr pair))
                                      (filter (lambda ([pair : (Pairof Action Category)])
                                                (equal? (car pair) (SUTTransition-action t)))
                                              cats))])
                     (map (lambda ([cat : Symbol]) (cons t cat)) (append default custom))))
                 (SUT-available-transitions s))]))

(: SUTCat-take-transition (-> SUTCatState SUTCatTransition Void))
(define/match (SUTCat-take-transition s t)
  [((cons ss sc) (cons ts tc))
   (SUT-take-transition ss ts)
   (match* (tc (SUTTransition-tid ts))
     [('NONE _) (void)]
     [('CAT_TASK_FINI id) (hash-remove! sc id)]
     [(_ id)
      (assert id RealTaskId?)
      (hash-set! sc id (Cat-take-transition (hash-ref sc id (lambda () 'CAT_NONE)) tc))])])

;; -----------------------------------------------------------------------------
;; SUTCat simulation
;; -----------------------------------------------------------------------------
(: SUTCat-generate-transition-chain
   (-> (Listof (Pairof Action Category))
       (-> (-> SUTCatState (Listof SUTCatTransition) SUTCatTransition)
           SUTCatState
           Nonnegative-Integer
           (Values SUTCatState (Listof SUTCatTransition)))))
(define ((SUTCat-generate-transition-chain cats) selector state length)
  (generate-transition-chain SUTCat-take-transition
                             (SUTCat-available-transitions cats)
                             selector
                             state
                             length))
