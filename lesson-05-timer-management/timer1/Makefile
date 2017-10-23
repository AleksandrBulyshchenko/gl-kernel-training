#
# Memory management example
#

ifneq ($(KERNELRELEASE),)

obj-m += xxx_last_read.o
obj-m += xxx_absolute.o

else

KERNELDIR := $(BUILD_KERNEL)

.PHONY: all clean
all:
	$(MAKE) -C $(KERNELDIR) M=$(CURDIR) modules
clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURDIR) clean

endif
