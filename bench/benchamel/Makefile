all: default

# ------------------------------------------------------------------------------
# commands and directories
# ------------------------------------------------------------------------------

BENCHMK= ../bench.mk
include ${BENCHMK}

# Lotto's build directory (better building Lotto in Release mode)
BUILD_DIR=	${PROJECT}/build

# Number of repetitions per bug
REPEAT=		1

# Compiler and linker configuration
CC=		gcc
CXX=		g++

# Lotto options and command
OPT_sampling=		--sampling-config ${ROOTDIR}/few-reads.conf
OPT_deadlock=		--enable deadlock --deadlock-check-extra-release
OPT_busyabort=		--enable busyabort --busyabort-threshold 1000
OPT_clocktune=		--clock-base-inc 1000
OPT_sleeponce=		--sleep-mode once
OPT_default=		--enable pos -s pos -a MASK -r 2000 \
			--memmgr-user uaf --mutex-strict

STRESS_CMD= 		${TIMED} -r ${REPEAT} -- \
			! ${BUILD_DIR}/lotto stress ${OPT_default}

# Use timedrun.sh in scripts directory
TIMED= 		${PROJECT}/scripts/timedrun.sh \
			-n $* -o ${WORKDIR}/$*.time
PARSE=		cat ${WORKDIR}/$*.time \
		| tee -a ${WORKDIR}/results.csv

# ROOTDIR is given by bench.mk, it refers to the directory of the benchmark
# PROJECT refers to the dice source directory
PROJECT!=	readlink -f ${ROOTDIR}/../..


# Add TARGET+=header to initialize the results.csv file
SUM.header=	echo 'bug run real user sys' > ${WORKDIR}/results.csv
TARGET+=	header

# ------------------------------------------------------------------------------
# clone and build benchamel
# ------------------------------------------------------------------------------

URL=		https://github.com/open-s4c/benchamel.git
TAG=		main

TARGET+=	clone
DIR.clone=	repo
CFG.clone=	git clone --branch ${TAG} --depth 1 ${URL} . && \
		git submodule update --init --recursive

TARGET+=	benchamel
DEP.benchamel=	.clone.cfg
DIR.benchamel=	build
CFG.benchamel=	cmake -S ../${DIR.clone} \
			-DBENCHAMEL_BUILD_TSAN=on \
			-DCMAKE_BUILD_TYPE=Debug \
			-DCMAKE_C_COMPILER=${CC} \
			-DCMAKE_CXX_COMPILER=${CXX}
BLD.benchamel=	${MAKE} -j 8
DEPBUILD=	.benchamel.bld

# ------------------------------------------------------------------------------
# Bug list and lotto tuning for specific bugs
# ------------------------------------------------------------------------------

OPT.convul.2016-9806=		${OPT_sleeponce}
OPT.convul.2017-15265=		${OPT_sleeponce} ${OPT_deadlock}
OPT.convul.2017-6346=		${OPT_sleeponce}

OPT.CB.cherokee-0.9.2=		${OPT_sampling} ${OPT_sleeponce}
OPT.CB.cherokee-bug1=		${OPT_sampling} ${OPT_sleeponce}
OPT.CB.httrack-3.43.9=		${OPT_sampling} ${OPT_clocktune}
OPT.CB.memcached-127=		${OPT_sleeponce}
OPT.CB.pbzip2-0.9.4= 		${OPT_sampling}
OPT.CB.transmission-1.42=	${OPT_sampling} ${OPT_clocktune}

OPT.vsync.bounded_mpmc_check_empty=	${OPT_busyabort}
OPT.vsync.hemlock=			${OPT_busyabort}
OPT.vsync.twalock=			${OPT_busyabort}

# list of ignored bugs (regex)
IGNORE=		inspect.* \
		convul.* \
		misc.* \
		vsync.* \
		chess.* \
		CS.* \
		CB.apache.* \
		CB.htt.*

# final (filtered) list of bugs
PAT!=		printf '%s\n' ${IGNORE} | paste -sd '|' -
_IGN=		grep -Ev '${PAT}'
_BUGS_LIST=	(cat ${WORKDIR}/${DIR.benchamel}/bugs.lst 2>/dev/null \
		|| printf '\n')
BUGS!=  	${_BUGS_LIST} | ${_IGN} | cut -d: -f1

# ------------------------------------------------------------------------------
# Per-bug target generic definitions
# The next variables only make sense if BUG is set, for example,
#    make run-bug BUG=X
# ------------------------------------------------------------------------------

# CMD.X contains the path to the binary of X taken from bugs.lst of benchamel
CMD.${BUG}!=	${_BUGS_LIST} | grep -e ${BUG}':' | cut -d':' -f2

# We now add a new target that can be executed with `make run TARGET=X BUG=X
TARGET+=	${BUG}
DEP.${BUG}=	${DEPBUILD}
DIR.${BUG}=	${DIR.benchamel}
RUN.${BUG}=	${STRESS_CMD} ${OPT.${BUG}} -- ${CMD.${BUG}}
SUM.${BUG}=	${PARSE}

# ------------------------------------------------------------------------------
# main make targets
# ------------------------------------------------------------------------------

default:
	@echo "=== Benchamel bugs ==="
	@echo ""
	@echo "== Usage: make <target>"
	@echo ""
	@echo "== Targets:"
	@echo "   build            download benchamel and build the binaries"
	@echo "   list-bugs        list all bugs"
	@echo "   run-all          run lotto with all bugs"
	@echo "   sum-all          summarize time measurements"
	@echo "   run-bug BUG=X    run lotto with bug X (see make list-bugs)"
	@echo ""
	@echo "== Notes:"
	@echo "   During a run, see the current output of bug X with"
	@echo "     tail -f ${WORKDIR}/.X.run.log"
	@echo ""
	@echo "   Summary results are stored in"
	@echo "     ${WORKDIR}/results.csv"

list-bugs:
	@${MAKE} -s build VERBOSE=${VERBOSE} FORCE=
	@printf "SELECTED BUGS:\n"
	@printf " %s\n" ${BUGS}
run-all:
	@${MAKE} -s build VERBOSE=${VERBOSE} FORCE=
	@for bug in ${BUGS}; do ${MAKE} -s run-bug BUG=$${bug} \
		VERBOSE=${VERBOSE} FORCE=${FORCE}; done
sum-all:
	@${MAKE} -s sum BUG='' TARGET=header VERBOSE=${VERBOSE}
	@for bug in ${BUGS}; do ${MAKE} -s sum-bug BUG=$${bug} \
		VERBOSE=${VERBOSE}; done
run-bug:
	@${MAKE} -s run TARGET=${BUG} BUG=${BUG} VERBOSE=${VERBOSE}
sum-bug: run-bug
	@${MAKE} -s sum TARGET=${BUG} BUG=${BUG} VERBOSE=${VERBOSE}
