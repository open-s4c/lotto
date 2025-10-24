#include <lotto/base/marshable.h>
#include <lotto/base/tidbag.h>
#include <lotto/sys/assert.h>

task_id data1[2];
task_id data2[4];
task_id *malloc_mock;
size_t malloc_expect;
bool running = false;

void *
malloc(size_t size)
{
    if (!running) {
        void *(*mlc)(size_t) = real_func("malloc", 0);
        return mlc(size);
    }

    ENSURE(size == malloc_expect);
    ENSURE(malloc_mock);
    void *r     = malloc_mock;
    malloc_mock = NULL;
    return r;
}

task_id *free_expect;
void
free(void *p)
{
    if (!running)
        return;
    ENSURE(p == free_expect);
}

void
test_insert()
{
    tidbag_t tset;
    malloc_mock   = data1;
    malloc_expect = TIDBAG_INIT_SIZE * sizeof(task_id);
    tidbag_init(&tset);
    ENSURE(tset.tasks == data1);

    tidbag_insert(&tset, 123);
    ENSURE(tset.size == 1);
    ENSURE(tset.tasks == data1);
    free_expect = data1;
    tidbag_fini(&tset);
}

void
test_expand()
{
    tidbag_t tset;
    malloc_mock   = data1;
    malloc_expect = 2 * sizeof(task_id);
    tidbag_init_cap(&tset, 2);

    tidbag_insert(&tset, 101);
    ENSURE(tset.size == 1);
    ENSURE(tset.capacity == 2);
    ENSURE(tset.tasks == data1);

    tidbag_insert(&tset, 102);
    ENSURE(tset.size == 2);
    ENSURE(tset.capacity == 2);
    ENSURE(tset.tasks == data1);

    free_expect   = data1;
    malloc_expect = 4 * sizeof(task_id);
    malloc_mock   = data2;
    tidbag_insert(&tset, 103);
    ENSURE(tset.size == 3);
    ENSURE(tset.capacity == 4);
    ENSURE(tset.tasks == data2);

    ENSURE(tidbag_has(&tset, 101));
    ENSURE(tidbag_has(&tset, 102));
    ENSURE(tidbag_has(&tset, 103));

    free_expect = data2;
    tidbag_fini(&tset);
}


void
test_get()
{
    tidbag_t tset = {0};
    tidbag_get(&tset, 0);
}

int
main()
{
    running = true;
    test_insert();
    test_expand();
    test_get();
    running = false;
    return 0;
}
