#
# mpu6050 accelerometer & gyroscope
# I2C monochrome 0.96" 128x64 OLED graphic display module with SSD1306 controller
#

#The option Module signature verification (CONFIG_MODULE_SIG) enables the module signature verification in the Linux kernel. It supports two approaches on signed module support: a rather permissive one and a strict one.
# see https://wiki.gentoo.org/wiki/Signed_kernel_module_support
CONFIG_MODULE_SIG=n

ifneq ($(KERNELRELEASE),)
obj-m := mpu6050.o ssd1306.o
else
ifeq ($(KERNELDIR),)
ifeq ($(BBB_KERNEL),)
	$(error Path to kernel tree - KERNELDIR or BBB_KERNEL variable is not defined!)
endif
endif

KERNELDIR ?= $(BBB_KERNEL)

export ARCH ?= arm
export CROSS_COMPILE ?= arm-linux-gnueabihf-

.PHONY: all clean config dtb modules clean_objects

all: modules analog dtb
	make clean_objects

modules:
	$(MAKE) -C $(KERNELDIR) M=$(CURDIR) modules
	make clean_objects

clean: clean_objects
	$(MAKE) -C $(KERNELDIR) M=$(CURDIR) clean
	rm -f analog_clock

config:
	$(MAKE) -C $(KERNELDIR) menuconfig

dtb:
	cp am335x-bone-common.dtsi $(KERNELDIR)/arch/arm/boot/dts/
	cp am335x-bone-common.dtsi $(KERNELDIR)/source/arch/arm/boot/dts/
	$(MAKE) -C $(KERNELDIR) M=$(CURDIR) dtbs

analog:
	$(CROSS_COMPILE)gcc -Wall -o analog_clock analog_clock.c -lm
	make clean_objects

clean_objects:
	rm -rf *.o *.*.o .*.cmd Module.* *.order .tmp_versions

endif
