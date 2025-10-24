#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lotto/base/marshable.h>
#include <lotto/base/tidmap.h>

/* my task object, eg, for PCT */
typedef struct {
    tiditem_t t;

    int my_additional_stuff;
    char msg[256];
} my_task_t;


void
test_marshal_tasks()
{
    tidmap_t p1, p2;

    /* initialize interface with size of my task object and its marshable
     * interface */
    tidmap_init(&p1, MARSHABLE_STATIC(sizeof(my_task_t)));
    tidmap_init(&p2, MARSHABLE_STATIC(sizeof(my_task_t)));

    /* register task 123 and get task object (also allocates my_t) */
    my_task_t *t1 = (my_task_t *)tidmap_register(&p1, 123);
    strcpy(t1->msg, "hello");

    /* register task 312 and get task object (also allocates my_t) */
    my_task_t *t2 = (my_task_t *)tidmap_register(&p1, 312);
    strcpy(t2->msg, "world");

    size_t s = marshable_size_m(&p1);
    assert(s == 2 * sizeof(my_task_t) + sizeof(tidmap_t));

    char *buf = (char *)malloc(s);
    assert(buf);

    char *b2 = (char *)marshable_marshal_m(&p1, buf);
    assert(buf + s == b2);


    char *b3 = (char *)marshable_unmarshal_m(&p2, buf);
    assert(buf + s == b3);

    t1 = (my_task_t *)tidmap_find(&p1, 123);
    t2 = (my_task_t *)tidmap_find(&p2, 123);
    assert(t1);
    assert(t2);
    assert(t1->t.key == t2->t.key);
    assert(strcmp(t1->msg, t2->msg) == 0);
    assert(strcmp(t1->msg, "hello") == 0);

    t1 = (my_task_t *)tidmap_find(&p1, 312);
    t2 = (my_task_t *)tidmap_find(&p2, 312);
    assert(t1);
    assert(t2);
    assert(t1->t.key == t2->t.key);
    assert(strcmp(t1->msg, t2->msg) == 0);
    assert(strcmp(t1->msg, "world") == 0);
}

int
main()
{
    test_marshal_tasks();
    return 0;
}
