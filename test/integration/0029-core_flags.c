// clang-format off
// RUN: rm -f %s.base.trace %s.config.trace %s.record.trace %s.log
// RUN: %lotto start -t %T -o %s.base.trace -- %b
// RUN: %lotto config -t %T -i %s.base.trace -o %s.config.trace --record-granularity CHPT,SWITCH --record-granularity CAPTURE
// RUN: %lotto show -t %T -i %s.config.trace | %check %s --check-prefix=GRAN
// RUN: (! %lotto stress -e busyabort -d deadlock -h 2>&1) | %check %s --check-prefix=MODULES
// RUN: (! %lotto stress -d all -e busyabort -h 2>&1) | %check %s --check-prefix=ALL
// RUN: (! %lotto stress -d busyabort -e busyabort -h 2>&1) | %check %s --check-prefix=LAST-ON
// RUN: (! %lotto stress -e busyabort -d busyabort -h 2>&1) | %check %s --check-prefix=LAST-OFF
// RUN: %lotto record -t %T -o %s.record.trace --log %s.log -vv -- %b > %s.out
// RUN: test -s %s.record.trace
// RUN: test -s %s.log
// RUN: %lotto show -t %T -i %s.record.trace | %check %s --check-prefix=INPUT
// RUN: (! %lotto record --record-granularity nope,SWITCH -h 2>&1) | %check %s --check-prefix=BADVALUE
//
// GRAN: gran  = SWITCH|CHPT|CAPTURE
// MODULES-DAG: busyabort{{[[:space:]]+}}on
// MODULES-DAG: deadlock{{[[:space:]]+}}off
// ALL-DAG: busyabort{{[[:space:]]+}}on
// ALL-DAG: pos{{[[:space:]]+}}off
// LAST-ON: busyabort{{[[:space:]]+}}on
// LAST-OFF: busyabort{{[[:space:]]+}}off
// INPUT: trace file: {{.*}}0029-core_flags.c.record.trace
// BADVALUE: invalid value for --record-granularity: nope,SWITCH
// clang-format on

#include <pthread.h>

static void *
thread(void *arg)
{
    (void)arg;
    return NULL;
}

int
main(void)
{
    pthread_t t;
    pthread_create(&t, NULL, thread, NULL);
    pthread_join(t, NULL);
    return 0;
}
