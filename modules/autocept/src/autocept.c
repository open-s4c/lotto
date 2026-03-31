#if !defined(__APPLE__)
    #include <dlfcn.h>
#endif

#include <dice/chains/intercept.h>
#include <dice/interpose.h>
#include <dice/pubsub.h>
#include <lotto/engine/pubsub.h>
#include <lotto/modules/autocept/events.h>

/* The assembly trampolines reserve a fixed stack slot for metadata_t and pass
 * its address to autocept_before/autocept_after. That means autocept currently
 * depends on the exact Dice metadata ABI size.
 *
 * If metadata_t changes, do not just update this assert. You must also:
 * - update the AUTOCEPT_MD_* size/offset constants in the arch asm backends
 * - recheck stack alignment around the helper calls
 * - recheck the metadata bit transport between BEFORE and AFTER
 * - rerun the interposed autocept tests, especially the checker path
 *
 * Keep this build-time guard here so Dice metadata ABI changes fail loudly
 * instead of silently corrupting the autocept stack frame layout.
 */
_Static_assert(sizeof(metadata_t) == 8,
               "autocept asm metadata slot size must match Dice metadata_t");

void *
autocept_before(struct autocept_call_event *ev, metadata_t *md)
{
    if (ev->type_id == ANY_EVENT)
        ev->type_id = EVENT_AUTOCEPT_CALL;
    PS_PUBLISH(INTERCEPT_BEFORE, ev->type_id, ev, md);

    if (ev->func == NULL) {
#if defined(__APPLE__)
        return NULL;
#else
        ev->func = real_sym(ev->name, 0);
#endif
    }

    return (void *)ev->func;
}

void
autocept_after(struct autocept_call_event *ev, metadata_t *md)
{
    if (ev->type_id == ANY_EVENT)
        ev->type_id = EVENT_AUTOCEPT_CALL;
    PS_PUBLISH(INTERCEPT_AFTER, ev->type_id, ev, md);
}
