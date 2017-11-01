KERNELDIR=/lib/modules/`uname -r`/build
#ARCH=i386
#KERNELDIR=/usr/src/kernels/`uname -r`-i686

EXTRA_CFLAGS += -I$(PWD)
MODULES = charDeviceDriver.ko
obj-m += charDeviceDriver.o

all: $(MODULES) 

charDeviceDriver.ko: charDeviceDriver.c ioctl.h
	make -C $(KERNELDIR) M=$(PWD) modules