LINUX_VERSION = 6.13.4
LINUX   = linux-${LINUX_VERSION}
BUSYBOX = busybox-1.36.1

LINUX_URL   = https://cdn.kernel.org/pub/linux/kernel/v6.x/${LINUX}.tar.xz
BUSYBOX_URL = https://busybox.net/downloads/${BUSYBOX}.tar.bz2

ROOTFS      = ${CURDIR}/rootfs
LINUX_IMAGE = ${LINUX}/arch/arm64/boot/Image
BUSYBOX_BIN = ${BUSYBOX}

MODULE  = example_mod
SOURCES = $(wildcard src/*.c)
TARGETS = $(addprefix ${ROOTFS}/bin/, $(notdir $(SOURCES:.c=)))

CROSS = aarch64-linux-gnu
CROSS_PRE = ${CROSS}-

SCX_PATHS = $(patsubst %, $(ROOTFS)/lib/%, $(SCX_LIBS))

INCLUDES = \
	-I src/include \
	-I ../../build/include \
	-I ../../include \
	-I ../../deps/dice/deps/libvsync/include \

help:
	@echo "Help:"
	@echo "- make get          download Linux and Busybox files"
	@echo "- make image        build Linux image"
	@echo "- make scx          build Linux bpf schedulers"
	@echo "- make busybox      build busybox tools"
	@echo "- make rootfs.img   build minimal root fs"

all: image busybox compile rootfs.img

${LINUX}.tar.xz:
	curl -kLO ${LINUX_URL}

${LINUX}: ${LINUX}.tar.xz
	tar xvf $<

${BUSYBOX}.tar.bz2:
	curl -kLO ${BUSYBOX_URL}

${BUSYBOX}: ${BUSYBOX}.tar.bz2
	tar jxvf $<

${LINUX}/.config: scx_defconfig
	cp $< $@

${BUSYBOX}/.config: ${BUSYBOX}.config
	cp $< $@

get: ${LINUX} ${BUSYBOX}

scx: ${LINUX}/.config
	patch --silent --forward -p 1 -d ${LINUX} -i ../patches/0001-Adds-scx_sched_rand-scheduler.patch || true
	patch --silent --forward -p 1 -d ${LINUX} -i ../patches/scx_cross_compile.patch || true
	make -C ${LINUX}/tools/sched_ext ARCH=arm64 CROSS_COMPILE=${CROSS_PRE}
	cp ${LINUX}/tools/sched_ext/build/bin/scx_sched_rand ${ROOTFS}/bin
	chmod a+x ${ROOTFS}/bin/scx_sched_rand

image: ${LINUX}/.config
	yes "" | make -C ${LINUX} ARCH=arm64 CROSS_COMPILE=${CROSS_PRE} oldconfig
	make -C ${LINUX} ARCH=arm64 CROSS_COMPILE=${CROSS_PRE}

busybox: ${BUSYBOX}/.config
	yes "" | make -C ${BUSYBOX} ARCH=arm64 CROSS_COMPILE=${CROSS_PRE} oldconfig
	make -C ${BUSYBOX} ARCH=arm64 CROSS_COMPILE=${CROSS_PRE}
	make -C ${BUSYBOX} ARCH=arm64 CROSS_COMPILE=${CROSS_PRE} \
		CONFIG_PREFIX=${ROOTFS} install

${ROOTFS}/lib/modules/${LINUX_VERSION}:
	mkdir -p $@
${ROOTFS}/bin:
	mkdir -p $@
${ROOTFS}:
	mkdir -p $@

${ROOTFS}/bin/%: src/%.c ${ROOTFS}/bin
	${CROSS_PRE}gcc $(INCLUDES) \
		-static -O0 -o $@ $< -lpthread

${ROOTFS}/lib/modules/${LINUX_VERSION}/${MODULE}.ko: modules/${MODULE}.c ${ROOTFS}/lib/modules/${LINUX_VERSION}
	cd modules && make all
	cp modules/${MODULE}.ko $@

${ROOTFS}/init.sh: src/init.sh ${ROOTFS}
	cp $< $@

compile: ${TARGETS} ${ROOTFS}/lib/modules/${LINUX_VERSION}/${MODULE}.ko

scx_libs: scx
	cp ${shell bash lib_deps.sh ${LINUX}/tools/sched_ext/build/bin/scx_sched_rand} ${ROOTFS}/lib

rootfs.img: compile ${ROOTFS}/init.sh scx_libs
	cd ${ROOTFS} && find . -print0 | cpio --null  -ov --format=newc > ../$@

clean:
	rm -rf rootfs
	make -C modules clean

.PHONY: image busybox get clean compile
