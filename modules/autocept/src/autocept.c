#include <dice/chains/intercept.h>
#include <dice/interpose.h>
#include <dice/pubsub.h>
#include <lotto/engine/pubsub.h>
#include <lotto/modules/autocept/events.h>

#if defined(__APPLE__)
    #include <mach-o/dyld.h>
    #include <string.h>
#endif

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

#if defined(__APPLE__)
static void *
autocept_real_sym(const char *name)
{
    void *func = real_sym(name, 0);
    if (func != NULL)
        return func;

    Dl_info self;
    const char *self_image = NULL;

    if (dladdr((void *)autocept_real_sym, &self) != 0)
        self_image = self.dli_fname;

    uint32_t count = _dyld_image_count();
    for (uint32_t i = 0; i < count; i++) {
        const char *image = _dyld_get_image_name(i);
        if (image == NULL ||
            (self_image != NULL && strcmp(image, self_image) == 0))
            continue;

        void *handle = dlopen(image, RTLD_LAZY | RTLD_NOLOAD | RTLD_FIRST);
        if (handle == NULL)
            continue;

        func = dlsym(handle, name);
        dlclose(handle);
        if (func != NULL)
            return func;
    }

    return NULL;
}
#else
static void *
autocept_real_sym(const char *name)
{
    return real_sym(name, 0);
}
#endif

void *
autocept_before(struct autocept_call_event *ev, metadata_t *md)
{
    if (ev->type_id == ANY_EVENT)
        ev->type_id = EVENT_AUTOCEPT_CALL;
    PS_PUBLISH(INTERCEPT_BEFORE, ev->type_id, ev, md);

    if (ev->func == NULL)
        ev->func = autocept_real_sym(ev->name);

    return (void *)ev->func;
}

void
autocept_after(struct autocept_call_event *ev, metadata_t *md)
{
    if (ev->type_id == ANY_EVENT)
        ev->type_id = EVENT_AUTOCEPT_CALL;
    PS_PUBLISH(INTERCEPT_AFTER, ev->type_id, ev, md);
}
