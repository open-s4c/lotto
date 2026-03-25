#lang typed/racket

(provide generate-transition-chain
         generate-random-transition-chain
         print-chain
         rnd-one-of)

(: rnd-one-of (All (a) (-> (Listof a) a)))
(define (rnd-one-of l)
  ;;  (printf "-Variants: ~a\n" l)
  (list-ref l (random 0 (length l))))

(: generate-transition-chain
   (All (state transition)
        (-> (-> state transition Void)
            (-> state (Listof transition))
            (-> state (Listof transition) transition)
            state
            Nonnegative-Integer
            (Values state (Listof transition)))))
(define (generate-transition-chain take-transition next-transitions selector state length)
  (let iter ([s
              :
              state
              state]
             [i
              :
              Nonnegative-Integer
              length]
             [acc
              :
              (Listof transition)
              '()])
    (if (= i 0)
        (values s (reverse acc))
        (let ([ts
               :
               (Listof transition)
               (next-transitions s)])
          (match ts
            ['() (values s (reverse acc))]
            [_
             (let ([t
                    :
                    transition
                    (selector s ts)])
               (take-transition s t)
               (iter s (- i 1) (cons t acc)))])))))

(: generate-random-transition-chain
   (All (state transition)
        (-> (-> state transition Void)
            (-> state (Listof transition))
            state
            Nonnegative-Integer
            (Values state (Listof transition)))))
(define (generate-random-transition-chain take-transition next-transitions state length)
  (generate-transition-chain take-transition
                             next-transitions
                             (lambda ([_ : state] [ts : (Listof transition)]) (rnd-one-of ts))
                             state
                             length))

(: print-chain
   (All (state transition) (-> (-> state transition Void) state (Listof transition) state)))
(define (print-chain tt s c)
  (printf "Event sequence:\n")
  (printf "~v " s)
  (foldl (lambda ([t : transition] [s : state])
           (printf "- ~v ->" t)
           (tt s t)
           (printf " ~v\n" s)
           s)
         s
         c))
