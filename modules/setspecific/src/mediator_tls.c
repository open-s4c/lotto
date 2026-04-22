#include <pthread.h>

#include <lotto/modules/setspecific/events.h>
#include <lotto/runtime/capture_point.h>
#include <lotto/runtime/mediator.h>

/*
 * Parking place for the old mediator-managed pthread TLS bookkeeping.
 *
 * This code was moved out of src/runtime/mediator.c because mediator should
 * not own EVENT_KEY_CREATE / EVENT_KEY_DELETE / EVENT_SET_SPECIFIC handling.
 * The intended future home is the setspecific module, but this file is not yet
 * wired into the build or adapted to the final module-local ownership model.
 */

#define MEDIATOR_VALUE_CAP      1024
#define MEDIATOR_DESTRUCTOR_CAP 1024

size_t ndestructors;
static struct mediator_destructor {
    pthread_key_t key;
    void (*destructor)(void *);
} destructors[MEDIATOR_DESTRUCTOR_CAP];

struct mediator_value {
    pthread_key_t key;
    void *value;
};

static void
mediator_tls_handle_capture(mediator_t *m, capture_point *cp)
{
    switch (cp->type_id) {
        case EVENT_KEY_CREATE:
            destructors[ndestructors++] = (struct mediator_destructor){
                .key        = *cp->key_create->key,
                .destructor = cp->key_create->destructor,
            };
            return;
        case EVENT_KEY_DELETE: {
            pthread_key_t key = cp->key_delete->key;
            for (size_t i = 0; i < ndestructors; i++) {
                if (destructors[i].key != key) {
                    continue;
                }
                destructors[i] = destructors[--ndestructors];
                break;
            }
            return;
        }
        case EVENT_SET_SPECIFIC: {
            /*
             * Old mediator-owned per-task pthread value storage lived here.
             * It was intentionally removed from mediator before this module was
             * integrated, so this code is left as a parking stub.
             */
            (void)m;
            return;
        }
        default:
            return;
    }
}

static void
mediator_tls_fini(mediator_t *m)
{
    /*
     * The old destructor replay logic also lived in mediator. Rehome it here
     * once setspecific grows an actual runtime-owned state object.
     */
    (void)m;
}
