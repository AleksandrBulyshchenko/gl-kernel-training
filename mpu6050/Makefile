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
KERNVER=$(shell $(MAKE) -C ${KERNELDIR} --no-print-directory kernelversion)

.PHONY: all clean deploy_mod dtb deploy_dtb

all:
	$(MAKE) -C $(KERNELDIR) M=$(CURDIR) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURDIR) clean

dtb:
	cp am335x-bone-common.dtsi $(KERNELDIR)/arch/arm/boot/dts/
	$(MAKE) -C $(KERNELDIR) M=$(CURDIR) dtbs

deploy_dtb: dtb #all
	scp ${KERNELDIR}/arch/arm/boot/dts/am335x-boneblack*.dtb root@beaglebone.local:/boot/dtbs/${KERNVER}/

deploy_mod: all
	scp *.ko root@beaglebone.local:

endif
