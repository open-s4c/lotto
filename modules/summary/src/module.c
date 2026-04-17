#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

#include "state.h"
#include <dice/chains/capture.h>
#include <dice/chains/intercept.h>
#include <dice/events/thread.h>
#include <dice/module.h>
#include <dice/pubsub.h>
#include <dice/self.h>
#include <lotto/engine/pubsub.h>
#include <lotto/sys/logger.h>
#include <lotto/sys/stdio.h>
#include <vsync/atomic/core.h>
#include <vsync/atomic/dispatch.h>

static vatomic64_t _counts[MAX_CHAINS][MAX_TYPES];
static vatomic32_t _reported;

static inline enum ps_err
_count_event(chain_id chain, type_id type)
{
    if (chain < MAX_CHAINS && type < MAX_TYPES) {
        vatomic_inc(&_counts[chain][type]);
    }
    return PS_OK;
}

#define SUMMARY_SUBSCRIBE(CHAIN_ID)                                            \
    PS_SUBSCRIBE(CHAIN_ID, ANY_EVENT, { return _count_event(chain, type); })

SUMMARY_SUBSCRIBE(CHAIN_CONTROL)
SUMMARY_SUBSCRIBE(INTERCEPT_EVENT)
SUMMARY_SUBSCRIBE(INTERCEPT_BEFORE)
SUMMARY_SUBSCRIBE(INTERCEPT_AFTER)
SUMMARY_SUBSCRIBE(CAPTURE_EVENT)
SUMMARY_SUBSCRIBE(CAPTURE_BEFORE)
SUMMARY_SUBSCRIBE(CAPTURE_AFTER)
SUMMARY_SUBSCRIBE(CHAIN_LOTTO_CONTROL)
SUMMARY_SUBSCRIBE(CHAIN_LOTTO_DEFAULT)
SUMMARY_SUBSCRIBE(CHAIN_INGRESS_EVENT)
SUMMARY_SUBSCRIBE(CHAIN_INGRESS_BEFORE)
SUMMARY_SUBSCRIBE(CHAIN_INGRESS_AFTER)
SUMMARY_SUBSCRIBE(CHAIN_SEQUENCER_CAPTURE)
SUMMARY_SUBSCRIBE(CHAIN_SEQUENCER_RESUME)

static void
_report_chain(chain_id chain)
{
    uint64_t chain_total = 0;
    for (type_id type = 0; type < MAX_TYPES; type++) {
        chain_total += vatomic_read(&_counts[chain][type]);
    }
    if (chain_total == 0) {
        return;
    }

    logger_println("[summary] chain=%u (%s) total=%" PRIu64, (unsigned)chain,
                   ps_chain_str(chain), chain_total);

    for (type_id type = 0; type < MAX_TYPES; type++) {
        uint64_t c = vatomic_read(&_counts[chain][type]);
        if (c == 0) {
            continue;
        }
        logger_println("[summary]   type=%u (%s) count=%" PRIu64,
                       (unsigned)type, ps_type_str(type), c);
    }
}

static void
_report_all()
{
    for (chain_id chain = 0; chain < MAX_CHAINS; chain++) {
        _report_chain(chain);
    }
}

static void
_report_csv(FILE *fp)
{
    sys_fprintf(fp, "chain,chain_name,type,type_name,count\n");
    for (chain_id chain = 0; chain < MAX_CHAINS; chain++) {
        for (type_id type = 0; type < MAX_TYPES; type++) {
            uint64_t c = vatomic_read(&_counts[chain][type]);
            if (c == 0) {
                continue;
            }
            sys_fprintf(fp, "%u,%s,%u,%s,%" PRIu64 "\n", (unsigned)chain,
                        ps_chain_str(chain), (unsigned)type, ps_type_str(type),
                        c);
        }
    }
}

DICE_MODULE_FINI({
    if (vatomic_xchg(&_reported, 1) == 0) {
        if (summary_config()->csv[0] != '\0') {
            FILE *fp = sys_fopen(summary_config()->csv, "w");
            if (fp == NULL) {
                logger_errorf("failed to open summary csv: %s\n",
                              summary_config()->csv);
                _report_all();
            } else {
                _report_csv(fp);
                sys_fclose(fp);
            }
        } else {
            _report_all();
        }
    }
})

LOTTO_MODULE_INIT()
