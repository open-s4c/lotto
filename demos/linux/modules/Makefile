LINUX   ?= linux-6.13.4

obj-m += example_mod.o
EXTRA_CFLAGS := -I$(CURDIR)/../../../build/include
EXTRA_CFLAGS += -I$(CURDIR)/../../../include
EXTRA_CFLAGS += -I$(CURDIR)/../../../modules/qemu/include
EXTRA_CFLAGS += -I$(CURDIR)/../../../modules/bias/include
EXTRA_CFLAGS += -I$(CURDIR)/../../../modules/terminate/include
EXTRA_CFLAGS += -I$(CURDIR)
EXTRA_CFLAGS += -Wno-strict-prototypes

all:
	make -C ../${LINUX} ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- M=$(CURDIR) modules

clean:
	make -C ../${LINUX} ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- M=$(CURDIR) clean
