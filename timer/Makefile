KERNELDIR := /home/mjs/Desktop/orangepi-build-main/kernel/orange-pi-4.9-sun50iw6
CURRENT_PATH := $(shell pwd)
KVERS := $(shell uname -r)

export ARCH=arm64
export CROSS_COMPILE=aarch64-linux-gnu-

obj-m += timer.o

build: kernel_modules

kernel_modules:
	make -C $(KERNELDIR) M=$(CURRENT_PATH) modules

clean:
	make -C $(KERNELDIR) M=$(CURRENT_PATH) clean
