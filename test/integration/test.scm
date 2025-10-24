(display "Running inflex_abc test from a Scheme script")
(newline)

(define program "./test/integration/inflex_abc")

(flags_set FLAG_ENGINE_STABLE_ADDRESS_METHOD "MASK")
(flags_set FLAG_ENGINE_SEED 1)
(cmd_set program)
(flags_set FLAG_CLI_ROUNDS 50)

(display (string-append "Program to run:" program))
(newline)

(define (call-n-times n proc) (do ((i 0 (+ i 1))) ((= i n)) (proc)))

(call-n-times 40 (lambda () (display "=")))
(newline)

(lotto stress)
;;(lotto inflex)
