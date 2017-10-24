#
# Memory management example
#

ifneq ($(KERNELRELEASE),)

obj-m += xxx.o
obj-m += xxx_kmalloc.o

else

KERNELDIR := $(BUILD_KERNEL)

.PHONY: all clean
all:
	$(MAKE) -C $(KERNELDIR) M=$(CURDIR) modules
clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURDIR) clean

endif
