#lang pollen
◊(require racket/string
          racket/format
	  "lah.rkt")

◊(define (print-options options)
  (string-join
    (for/list ([opt options])
          (format "- `~a`: (default `~a`)\n\n  ~a\n"
              (cmake-option-name opt)
              (cmake-option-defv opt)
              (cmake-option-desc opt)))
    "\n"))
◊(define options
    (let ([args (current-command-line-arguments)])
        (when (zero? (vector-length args))
            (error 'no-input-file))
        (let ([fn (vector-ref args 2)])
            (read-options fn))))
# Lotto build options

◊(print-options (select-options options "LOTTO"))

# Further options

## Libvsync options

◊(print-options (select-options options "LIBVSYNC"))

## Capstone options

◊(print-options (select-options options "CAPSTONE"))

## Lit options

◊(print-options (select-options options "LIT_"))
