# Lotto's build directory (better building Lotto in Release mode)
BUILD_DIR=	${PROJECT}/build

# Number of repetitions per bug
REPEAT=		1

# Compiler and linker configuration
CC=		gcc
CXX=		g++

# ------------------------------------------------------------------------------
# commands and directories
# ------------------------------------------------------------------------------
STRESS_CMD= 	${TIMED} -r ${REPEAT} -- ! ${BUILD_DIR}/lotto stress \
						--stable-address-method MASK \
						--memmgr-user uaf \
						-r 2000
# Use timedrun.sh in scripts directory
TIMED= 		${PROJECT}/scripts/timedrun.sh \
			-n $* -o ${WORKDIR}/$*.time
PARSE=		cat ${WORKDIR}/$*.time \
		| tee -a ${WORKDIR}/results.csv

# ROOTDIR is given by bench.mk, it refers to the directory of the benchmark
# PROJECT refers to the dice source directory
PROJECT!=	readlink -f ${ROOTDIR}/../..
PROJECT=	$(shell readlink -f ${ROOTDIR}/../..)


# Add TARGET+=header to initialize the results.csv file
PRO.header=	echo 'bug run real user sys' > ${WORKDIR}/results.csv
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
CFG.benchamel=	cmake -S ../${DIR.clone} -DBENCHAMEL_BUILD_TSAN=on \
					-DCMAKE_C_COMPILER=${CC} \
					-DCMAKE_CXX_COMPILER=${CXX}
BLD.benchamel=	${MAKE} -j 8
DEPBUILD=	.benchamel.bld

# ------------------------------------------------------------------------------
# test programs
# ------------------------------------------------------------------------------

# bug-specific options passed to Lotto stress
OPT.CB.pbzip2= 			--sampling-config ${ROOTDIR}/few-reads.conf
OPT.CB.aget= 			--sampling-config ${ROOTDIR}/few-reads.conf
OPT.CS.twostage_100_bad= 	--handler-deadlock off

# pbzip2 is not realiably working


BUGS=	\
	convul.2009-3547 \
	convul.2011-2183 \
	convul.2013-1792 \
	convul.2015-7550 \
	convul.2016-1972 \
	convul.2016-1973 \
	convul.2016-7911 \
	convul.2016-9806 \
	convul.2017-15265 \
	convul.2017-6346 \
	vsync.bounded_mpmc_check_empty \
	vsync.bounded_spsc \
	vsync.skiplist \
	vsync.caslock \
	vsync.clhlock \
	vsync.hemlock \
	vsync.twalock \
	chess.InterlockedWorkStealQueue \
	chess.InterlockedWorkStealQueueWithState \
	chess.StateWorkStealQueue \
	chess.WorkStealQueue \
	CB.aget \
	CB.stringbuffer \
	CS.account_bad \
	CS.arithmetic_prog_bad \
	CS.bluetooth_driver_bad \
	CS.carter01_bad \
	CS.circular_buffer_bad \
	CS.deadlock01_bad \
	CS.din_phil2_sat \
	CS.din_phil3_sat \
	CS.din_phil4_sat \
	CS.din_phil5_sat \
	CS.din_phil6_sat \
	CS.din_phil7_sat \
	CS.fsbench_bad \
	CS.lazy01_bad \
	CS.phase01_bad \
	CS.queue_bad \
	CS.reorder_10_bad \
	CS.reorder_20_bad \
	CS.reorder_3_bad \
	CS.reorder_4_bad \
	CS.reorder_5_bad \
	CS.stack_bad \
	CS.sync01_bad \
	CS.sync02_bad \
	CS.token_ring_bad \
	CS.twostage_100_bad \
	CS.twostage_bad \
	CS.wronglock_3_bad \
	CS.wronglock_bad \
	misc.freebsd_buf_ring01 \
	misc.freebsd_buf_ring02

# ------------------------------------------------------------------------------
# main make targets
# ------------------------------------------------------------------------------

all:
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
list-bugs: build
	@echo ${BUGS}
run-all: build
	@for bug in ${BUGS}; do ${MAKE} -s run-bug BUG=$${bug} VERBOSE=${VERBOSE}; done
sum-all:
	@${MAKE} -s sum BUG='' TARGET=header VERBOSE=${VERBOSE}
	@for bug in ${BUGS}; do ${MAKE} -s sum-bug BUG=$${bug} VERBOSE=${VERBOSE}; done
run-bug:
	@${MAKE} -s run TARGET=${BUG} BUG=${BUG} VERBOSE=${VERBOSE}
sum-bug: run-bug
	@${MAKE} -s process TARGET=${BUG} BUG=${BUG} VERBOSE=${VERBOSE}

# ------------------------------------------------------------------------------
# The next variables only make sense if BUG is set, for example,
#    make run-bug BUG=X

# CMD.X contains the path to the binary of X taken from bugs.lst of benchamel
CMD.${BUG}=	$(shell cat ${WORKDIR}/${DIR.benchamel}/bugs.lst | grep -e ${BUG}':' | cut -d':' -f2)
ABS.${BUG}=	$(shell dirname ${CMD.${BUG}})

# We now add a new target that can be executed with `make run TARGET=X BUG=X
TARGET+=	${BUG}
DEP.${BUG}=	${DEPBUILD}
DIR.${BUG}=	${DIR.benchamel}
RUN.${BUG}=	cd ${ABS.${BUG}} && ${STRESS_CMD} ${OPT.${BUG}} -- ${CMD.${BUG}}
PRO.${BUG}=	${PARSE}

# ------------------------------------------------------------------------------
BENCHMK= ../bench.mk
include ${BENCHMK}
