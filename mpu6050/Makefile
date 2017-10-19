#
# mpu6050 accelerometer & gyroscope
#
ifneq ($(KERNELRELEASE),)

obj-m := mpu6050.o

else


ifeq ($(KERNELDIR),)
ifeq ($(BBB_KERNEL),)
	$(error Path to kernel tree - KERNELDIR or BBB_KERNEL variable is not defined!)
endif
endif

KERNELDIR ?= $(BBB_KERNEL)

export ARCH = arm
export CROSS_COMPILE ?= arm-linux-gnueabihf-


.PHONY: all clean dtb

all:
	$(MAKE) -C $(KERNELDIR) M=$(CURDIR) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURDIR) clean

dtb:
	cp am335x-bone-common.dtsi $(KERNELDIR)/arch/arm/boot/dts/
	$(MAKE) -C $(KERNELDIR) M=$(CURDIR) dtbs


endif
