add_subdirectory(mock)

add_racket_test(FILENAME deadlock_test.rkt ARG DT_DEADLOCK)
add_racket_test(FILENAME deadlock_test.rkt ARG DT_EXTRA_UNLOCK)
add_racket_test(FILENAME deadlock_test.rkt ARG DT_LOST_LOCK)
add_racket_test(FILENAME engine_test.rkt)
add_racket_test(FILENAME engine_prop_test.rkt)
add_racket_test(FILENAME replay_test.rkt)
