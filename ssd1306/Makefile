CONFIG_MODULE_SIG=n

ifneq ($(KERNELRELEASE),)

obj-m := ssd1306.o

else

ifeq ($(KERNELDIR),)
ifeq ($(BBB_KERNEL),)
	$(error Path to kernel tree - KERNELDIR or BBB_KERNEL variable is not defined!)
endif
endif

export ARCH = arm
KERNELDIR ?= $(BBB_KERNEL)
KERNVER=$(shell $(MAKE) -C ${KERNELDIR} --no-print-directory kernelversion)

PWD := $(shell pwd)

.PHONY: all clean config dtb deploy_dtb deploy_mod

all:
		$(MAKE) ARCH=arm  -C $(KERNELDIR) M=$(PWD) modules

clean:
		$(MAKE) ARCH=arm  -C $(KERNELDIR) SUBDIRS=$(PWD) clean

config:
		$(MAKE) ARCH=arm -C $(KERNELDIR) menuconfig

dtb:
		cp ../bbb_common/am335x-bone-common.dtsi $(KERNELDIR)/arch/arm/boot/dts/
		$(MAKE) -C $(KERNELDIR) M=$(PWD) dtbs		
		
deploy_dtb: dtb
		scp ${KERNELDIR}/arch/arm/boot/dts/am335x-boneblack*.dtb ${bbb}:/boot/dtbs/${KERNVER}/

deploy_mod: all
		scp *.ko ${bbb}:

endif
