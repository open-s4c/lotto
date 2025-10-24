#lang racket/base

(require racket/string
         racket/file
         parsack)

(struct cmake-option [name type defv desc] #:transparent)

;; -----------------------------------------------------------------------------
;; parser rules
;; -----------------------------------------------------------------------------
(define $end (<or> $eol $eof $newline))
(define $line (many (noneOf "\n")))

(define $comment-line
  (parser-compose (ln <- (>> (string "// ") $line)) ;
                  $end
                  (return ln)))

(define $commented-option
  (parser-compose (desc <- (many $comment-line))
                  (name <- $identifier)
                  (char #\:)
                  (type <- $identifier)
                  (char #\=)
                  (defv <- (many (noneOf "\n")))
                  $end
                  (return (cmake-option (list->string name)
                                        (list->string type)
                                        (list->string defv)
                                        (string-join (map list->string desc) " ")))))

(define $cmake-message (>> (string "-- ") (>> (many (noneOf "\n")) (>> $end (return null)))))

(define $commented-options
  (>> (many $cmake-message) ;
      (many (parser-compose (opt <- $commented-option) (<or> $end (string "\n")) (return opt)))))

;; -----------------------------------------------------------------------------
;; parse and generate option objects
;; -----------------------------------------------------------------------------
(define (parse-options str)
  (parse-result $commented-options str))

(module+ test

  (parse-options #<<EOF
-- something
-- something else
EOF
                 )
  (parse-options #<<EOF
-- something
-- something else
// Use TSAN in Lotto runtime1
// Use TSAN in Lotto runtime2
LOTTO_TSAN_RUNTIME:BOOL=ON

// Use another
VAR:TYPE=VAL
EOF
                 ))

(define (read-options fn)
  (when (not (file-exists? fn))
    (printf "file not found: ~a\n" fn)
    (error 'file-not-found))
  (parse-options (file->string fn)))
(define (select-options options rg)
  (filter (λ (opt) (regexp-match rg (cmake-option-name opt))) options))

(provide cmake-option
         cmake-option-name
         cmake-option-defv
         cmake-option-desc
         cmake-option-type
         select-options
         read-options)

#|
(let ([args (current-command-line-arguments)])
  (when (not (zero? (vector-length args)))
    ;  (error 'no-input-file)
    (let ([fn (vector-ref args 0)])
      (when (not (file-exists? fn))
        (error 'file-not-found))
      (for ([opt (parse-options (file->string fn))])
        (displayln opt)))))
|#
