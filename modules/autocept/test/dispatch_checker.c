#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <dice/chains/capture.h>
#include <dice/module.h>
#include <dice/pubsub.h>
#include <dice/self.h>
#include <lotto/modules/autocept/events.h>

static bool seen_before;

static void
report(const char *msg)
{
    size_t len = strlen(msg);
    ssize_t ret = write(STDOUT_FILENO, msg, len);
    (void)ret;
}

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_AUTOCEPT_CALL, {
    const struct autocept_call_event *ev = EVENT_PAYLOAD(event);

    assert(strcmp(ev->name, "dispatch_semaphore_wait") == 0);
    seen_before = true;
    report("dispatch_semaphore_wait before\n");
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_AFTER, EVENT_AUTOCEPT_CALL, {
    const struct autocept_call_event *ev = EVENT_PAYLOAD(event);

    assert(seen_before);
    assert(strcmp(ev->name, "dispatch_semaphore_wait") == 0);
    report("dispatch_semaphore_wait after\n");
    return PS_OK;
})
