#include <alloca.h>
#include <poll.h>
#include <signal.h>
#include <time.h>

#include <lotto/base/callrec.h>
#include <lotto/base/record.h>
#include <lotto/base/trace.h>
#include <lotto/engine/prng.h>
#include <lotto/engine/pubsub.h>
#include <lotto/engine/recorder.h>
#include <lotto/modules/clock.h>
#include <lotto/runtime/ingress.h>
#include <lotto/sys/logger.h>
#include <lotto/sys/real.h>
#include <lotto/sys/sched.h>
#include <lotto/unsafe/time.h>
#include <lotto/util/casts.h>
#include <sys/types.h>

ssize_t
getrandom(void *buf, size_t buflen, unsigned int flags)
{
    if (buflen == 0)
        return 0;

    ASSERT(buf != NULL);

    uint32_t buf_idx = 0;
    uint8_t *buf_p   = (uint8_t *)buf;

    uint64_t seed = prng_next();
    uint64_t r    = 0;

    while (buf_idx < buflen) {
        if (buf_idx % 8 == 0) {
            r = lcg_next(seed);
        }
        buf_p[buf_idx] = CAST_TYPE(uint8_t, r & 0xFF);
        r              = r >> 8;
        buf_idx++;
    }

    return buf_idx;
}
