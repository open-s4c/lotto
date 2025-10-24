add_subdirectory(mock)

add_racket_test(
    FILENAME plan_test.rkt
    BINDING lotto/engine/plan.h
    DEPENDENCIES base_component)

add_racket_test(
    FILENAME tidset_test.rkt
    BINDING lotto/engine/tidset.h
    DEPENDENCIES base_component)

add_racket_test(FILENAME bag_test.rkt)

add_racket_test(FILENAME tidmap_test.rkt)

add_racket_test(FILENAME tidbag_test.rkt)

add_racket_test(FILENAME trace_flat_test.rkt)
