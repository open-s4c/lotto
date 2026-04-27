#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
. "$SCRIPT_DIR/lib/common.sh"

SERVICE_NAME="${SERVICE_NAME:-netbsd-bug-58561}"
SERVICE_DIR="$(netbsd_reset_service_dir "$SERVICE_NAME")"
RUNNER_CFLAGS="${RUNNER_CFLAGS:-}"

netbsd_require_tools
netbsd_write_qlotto_sources "$SERVICE_DIR"

if [ "${QLOTTO_SNAPSHOT:-0}" = "1" ]; then
    RUNNER_CFLAGS="${RUNNER_CFLAGS} -DQLOTTO_SNAPSHOT_ENABLED=1"
fi

cat >"$SERVICE_DIR/options.mk" <<'EOF'
IMGSIZE=2048
SETS=base.tar.xz etc.tar.xz comp.tar.xz
EOF

cat >"$SERVICE_DIR/etc/rc" <<EOF
#!/bin/sh

. /etc/include/basicrc

echo "${SERVICE_NAME}: booting"

status=1
if [ -x /usr/local/bin/qlotto-run ] && [ -x /usr/local/bin/bug-58561-run.sh ]; then
	/usr/local/bin/qlotto-run /usr/local/bin/bug-58561-run.sh
	status=\$?
else
	echo "${SERVICE_NAME}: missing qlotto runner or workload"
fi

if [ "\$status" -eq 0 ]; then
	echo "${SERVICE_NAME}: EXIT_SUCCESS"
else
	echo "${SERVICE_NAME}: EXIT_FAIL status=\$status"
fi

sync
poweroff || halt

. /etc/include/shutdown
EOF

cat >"$SERVICE_DIR/postinst/00-build.sh" <<EOF
#!/bin/sh
set -eu

rootdir="usr/src/bug-58561"
bindir="usr/local/bin"

mkdir -p "\${rootdir}" "\${bindir}"

cc -O2 ${RUNNER_CFLAGS} -o "\${bindir}/qlotto-run" \
	-I../service/${SERVICE_NAME} \
	../service/${SERVICE_NAME}/qlotto_run_command.c

cat > "\${bindir}/bug-58561-run.sh" <<'INNER'
#!/bin/sh
set -eu

workdir=/usr/src/bug-58561
jobs=\${BUG_58561_JOBS:-4}
iters=\${BUG_58561_ITERS:-25}

echo "bug-58561: running compile storm jobs=\${jobs} iterations=\${iters}"
cd "\${workdir}"

i=1
while [ "\$i" -le "\$iters" ]; do
	echo "bug-58561: iteration \${i}/\${iters}"
	make clean >/dev/null
	make -j"\${jobs}" all
	i=\$((i + 1))
done

echo "bug-58561: workload completed"
INNER
chmod 755 "\${bindir}/bug-58561-run.sh"

cat > "\${rootdir}/Makefile" <<'INNER'
PROG=bug58561
SRCS=main.c
SRCS+= worker00.c worker01.c worker02.c worker03.c worker04.c worker05.c worker06.c worker07.c
SRCS+= worker08.c worker09.c worker10.c worker11.c worker12.c worker13.c worker14.c worker15.c
SRCS+= worker16.c worker17.c worker18.c worker19.c worker20.c worker21.c worker22.c worker23.c
SRCS+= worker24.c worker25.c worker26.c worker27.c worker28.c worker29.c worker30.c worker31.c
CPPFLAGS+=-D_POSIX_C_SOURCE=200809L
CFLAGS+=-O2 -pipe
LDFLAGS+=-pthread

all: \${PROG}

\${PROG}: \${SRCS:.c=.o}
	\${CC} \${LDFLAGS} -o \${.TARGET} \${.ALLSRC}

clean:
	rm -f \${PROG} *.o
INNER

cat > "\${rootdir}/main.c" <<'INNER'
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

extern uint64_t worker00(uint64_t);
extern uint64_t worker01(uint64_t);
extern uint64_t worker02(uint64_t);
extern uint64_t worker03(uint64_t);
extern uint64_t worker04(uint64_t);
extern uint64_t worker05(uint64_t);
extern uint64_t worker06(uint64_t);
extern uint64_t worker07(uint64_t);
extern uint64_t worker08(uint64_t);
extern uint64_t worker09(uint64_t);
extern uint64_t worker10(uint64_t);
extern uint64_t worker11(uint64_t);
extern uint64_t worker12(uint64_t);
extern uint64_t worker13(uint64_t);
extern uint64_t worker14(uint64_t);
extern uint64_t worker15(uint64_t);
extern uint64_t worker16(uint64_t);
extern uint64_t worker17(uint64_t);
extern uint64_t worker18(uint64_t);
extern uint64_t worker19(uint64_t);
extern uint64_t worker20(uint64_t);
extern uint64_t worker21(uint64_t);
extern uint64_t worker22(uint64_t);
extern uint64_t worker23(uint64_t);
extern uint64_t worker24(uint64_t);
extern uint64_t worker25(uint64_t);
extern uint64_t worker26(uint64_t);
extern uint64_t worker27(uint64_t);
extern uint64_t worker28(uint64_t);
extern uint64_t worker29(uint64_t);
extern uint64_t worker30(uint64_t);
extern uint64_t worker31(uint64_t);

static uint64_t (*const funcs[])(uint64_t) = {
    worker00, worker01, worker02, worker03, worker04, worker05, worker06, worker07,
    worker08, worker09, worker10, worker11, worker12, worker13, worker14, worker15,
    worker16, worker17, worker18, worker19, worker20, worker21, worker22, worker23,
    worker24, worker25, worker26, worker27, worker28, worker29, worker30, worker31,
};

struct thread_arg {
    unsigned index;
    uint64_t seed;
};

static void *
run_thread(void *cookie)
{
    struct thread_arg *arg = cookie;
    uint64_t value = arg->seed;

    for (size_t i = 0; i < (sizeof(funcs) / sizeof(funcs[0])); i++)
        value ^= funcs[(i + arg->index) % (sizeof(funcs) / sizeof(funcs[0]))](value + i);

    return (void *)(uintptr_t)value;
}

int
main(void)
{
    pthread_t tids[8];
    struct thread_arg args[8];
    uint64_t total = 0;

    for (unsigned i = 0; i < 8; i++) {
        args[i].index = i;
        args[i].seed = 0x12345678ULL + (uint64_t)i * 0x10001ULL;
        if (pthread_create(&tids[i], NULL, run_thread, &args[i]) != 0) {
            perror("pthread_create");
            return 1;
        }
    }

    for (unsigned i = 0; i < 8; i++) {
        void *ret = NULL;
        if (pthread_join(tids[i], &ret) != 0) {
            perror("pthread_join");
            return 1;
        }
        total ^= (uint64_t)(uintptr_t)ret;
    }

    printf("bug58561 total=%llu\n", (unsigned long long)total);
    return total == 0 ? 1 : 0;
}
INNER

i=0
while [ "\$i" -lt 32 ]; do
	name=\$(printf 'worker%02d.c' "\$i")
	next1=\$(( (\$i + 1) % 32 ))
	next2=\$(( (\$i + 7) % 32 ))
	cat > "\${rootdir}/\${name}" <<INNER
#include <stdint.h>

static uint64_t
mix_\${i}(uint64_t x)
{
    for (unsigned j = 0; j < 4000; j++) {
        x ^= (x << 7) | (x >> 3);
        x += 0x9e3779b97f4a7c15ULL + (uint64_t)j + \${i}ULL;
        x ^= x >> 11;
    }
    return x;
}

uint64_t
worker\$(printf '%02d' "\$i")(uint64_t x)
{
    x = mix_\${i}(x);
    x ^= mix_\${i}(x + \${next1}ULL);
    x += mix_\${i}(x ^ \${next2}ULL);
    return x ^ \${i}ULL;
}
INNER
	i=\$((i + 1))
done

chmod 755 "\${bindir}/qlotto-run"
EOF

chmod 755 "$SERVICE_DIR/etc/rc" "$SERVICE_DIR/postinst/00-build.sh"

netbsd_build_service "$SERVICE_NAME"
netbsd_print_artifact_summary "$SERVICE_NAME"
