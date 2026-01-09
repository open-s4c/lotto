/*
 */
#ifndef LOTTO_CLI_CONSTRAINTS_H
#define LOTTO_CLI_CONSTRAINTS_H

#include <limits.h>

#include "ocset.h"
#include <lotto/base/envvar.h>
#include <lotto/base/flags.h>
#include <lotto/base/record.h>
#include <lotto/cli/args.h>

bool get_event(event_c *result, const record_t *record, const char *trace);

bool event_compatible(const event_c *source, const event_c *target);
bool constraint_equals(const constraint_t *lhs, const constraint_t *rhs);
bool event_equals(const event_c *lhs, const event_c *rhs);
bool constraint_set_contains_all(const constraint_t *A, uint64_t a,
                                 const constraint_t *B, uint64_t b);
bool constraint_set_contains_constraint(const constraint_t *constraint_set,
                                        uint64_t size,
                                        const constraint_t *constraint);
bool constraint_set_contains_event(const event_c *event,
                                   const constraint_t *constraint_set,
                                   uint64_t size);
bool constraint_set_satisfies_trace(const constraint_t *constraint_set,
                                    uint64_t size, const char *trace);
bool constraint_set_does_not_satisfy_trace(const constraint_t *constraint_set,
                                           uint64_t size, const char *trace);
bool find_in_trace(const event_c *event, const char *trace,
                   event_c *out_found_event, record_t **out_record);
bool event_in_trace(const event_c *event, const char *trace);
clk_t clk_in_trace(const event_c *event, const char *trace);
bool event_from_clock_full_trace(event_c *result, clk_t clk, const char *trace,
                                 bool first);
bool event_from_clock(event_c *result, args_t *args, flags_t *flags, clk_t clk,
                      const char *trace, bool first);

#endif
