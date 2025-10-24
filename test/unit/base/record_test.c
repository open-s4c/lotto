#include <lotto/base/record.h>
#include <lotto/sys/ensure.h>


void
test()
{
    record_t *reco = record_alloc(10);
    record_t *tmp  = record_clone(reco);
    ENSURE(reco->size == tmp->size);
}


int
main()
{
    test();
    return 0;
}
