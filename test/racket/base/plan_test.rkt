#lang racket

(require quickcheck
         rackunit/quickcheck
         rackunit
         ffi/unsafe)

;; -----------------------------------------------------------------------------
;; ctype definitions and cfunc registrations
;; -----------------------------------------------------------------------------

(require "mock/plan.rkt")
(load-lib "libbase_component")

;; -----------------------------------------------------------------------------
;; plan specification
;; -----------------------------------------------------------------------------

;; plans allowed in the correct sequence of actions
(define allowed-plans
  ;; CAT_CALL
  ;; CAT_TASK_CREATE
  '((WAKE CALL RETURN YIELD RESUME) (CALL YIELD RESUME)
                                    ;; CAT_TASK_INIT
                                    (WAKE YIELD RESUME)
                                    ;; CAT_TASK_JOIN
                                    (WAKE)
                                    ;; Otherwise
                                    (WAKE YIELD RESUME)
                                    ;; or
                                    (CONTINUE)
                                    ;; or even
                                    (RESUME CONTINUE)))

;; transform a symbol X in ACTION_X, eg, WAKE -> ACTION_WAKE
(define (action-sym sym)
  (let ([str (symbol->string sym)]) (string->symbol (string-append "ACTION_" str))))

;; convert the allowed plans in their actual enum symbol
(define allowed-plans-sym (map (lambda (l) (map action-sym l)) allowed-plans))

;; find the integer value of an enum symbol
(define (action->integer a)
  (cast a _action_t _ufixint))

;; conver the allowed plans to a list of integers. The actual values used
;; in the plan_t object
(define allowed-plans-vals
  (map (lambda (l)
         (define as-int (map action->integer l))
         (foldr + 0 as-int))
       allowed-plans-sym))

;; -----------------------------------------------------------------------------
;; helper function
;; -----------------------------------------------------------------------------
(define (reverse list)
  (foldl cons null list))

;; transform a plan integer into a sequence of enum symbols as dictated by
;; plan_next and plan_done C functions.
(define (expand-actions plan)
  (define (iter plan action-list)
    (define action (call plan_next plan))
    (if (equal? action 'ACTION_NONE)
        (reverse action-list)
        (let ([alist (cons action action-list)])
          (call plan_done plan)
          (iter plan alist))))
  (iter plan '()))

;; an action number is valid if it expands to one of the plans in
;; allowed-plans-sym
(define (valid-sequence actions)
  (define p (alloc-plan))
  (set-plan-actions! p actions)
  (define actions-list (expand-actions p))
  (define (one-of l)
    (cond
      [(empty? l) #f]
      [(equal? (first l) actions-list) #t]
      [else (one-of (rest l))]))
  (one-of allowed-plans-sym))

;; -----------------------------------------------------------------------------
;; test
;; -----------------------------------------------------------------------------

;; check plan_done and plan_next together with the definition of action enum.
(check-property (property ([actions arbitrary-natural])
                          ;; correct if the action number is not a possible one
                          ;; (we ignore) or the action number translates to
                          ;; a valid sequence.
                          (if (not (member actions allowed-plans-vals))
                              #t
                              (valid-sequence actions))))

(test-case "plan_has works (positive case)"
  (for ([plan allowed-plans-sym]
        [pnum allowed-plans-vals])
    (define p (alloc-plan))
    (set-plan-actions! p pnum)
    (for ([action plan])
      (check-true (call plan_has p action)))))
