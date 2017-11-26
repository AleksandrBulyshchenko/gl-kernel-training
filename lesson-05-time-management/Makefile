#
# Memory management example
#

ifneq ($(KERNELRELEASE),)

obj-m += xxx_1.o
obj-m += xxx_2.o
obj-m += xxx_timer.o
obj-m += xxx_hrtimer.o

else

KERNELDIR := $(BUILD_KERNEL)

.PHONY: all clean
all:
	$(MAKE) -C $(KERNELDIR) M=$(CURDIR) modules
clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURDIR) clean

endif
