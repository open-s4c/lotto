#lang racket

(define rx-filepath "^[\t ]*([/a-zA-Z0-9._-]+) *")
(define rx-reladdr "\\(([a-zA-Z0-9_]*\\+0x[0-9a-f]+)\\) *")
(define rx-absaddr "\\[(0x[0-9a-f]+)\\]")

;; extract binary path, relative addr and absolute addr
(define (extract-trio ln)
  (list (regexp-match rx-filepath ln) ;
        (regexp-match rx-reladdr ln)
        (regexp-match rx-absaddr ln)))

;;(let ([example-line "/tmp/liblotto/libplotto.so(__assert_fail+0x101)[0x7ffff7f7ddd8]"])
;;  (displayln (extract-line example-line)))

(define (addr2line line)
  (if (and (list? line) (= (length line) 3)) ;
      (system (string-append "addr2line -e " (first line) " " (second line)))
      #f))

(define (is-valid trio)
  (and (list? trio)
       (= (length trio) 3)
       (for/and ([lst trio])
         (and (list? lst) (> (length lst) 1)))))

(define (translate-line ln)
  (let ([trio (extract-trio ln)])
    (and (is-valid trio) ;
         (addr2line (map second trio)))))

(define (process-line ln)
  (define mt (regexp-match (string-append rx-filepath rx-reladdr rx-absaddr) ln))
  (when (not (translate-line ln))
    (printf "~a\n" ln)))

(let loop ()
  (define ln (read-line (current-input-port) 'any))
  (when (not (eof-object? ln))
    (process-line ln)
    (loop)))
