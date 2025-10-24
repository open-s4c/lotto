LINUX   ?= linux-6.13.4

obj-m += example_mod.o
EXTRA_CFLAGS := -I$(PWD)/../../../build/include
EXTRA_CFLAGS += -I$(PWD)/../../../include
EXTRA_CFLAGS += -I$(PWD)
EXTRA_CFLAGS += -Wno-strict-prototypes

all:
	make -C ../${LINUX} ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- M=$(PWD) modules

clean:
	make -C ../${LINUX} ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- M=$(PWD) clean
