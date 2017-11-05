#
# Memory management example
#

ifneq ($(KERNELRELEASE),)

obj-m += xxx_km.o xxx_sa.o

else

KERNELDIR := $(BUILD_KERNEL)

.PHONY: all clean
all:
	$(MAKE) -C $(KERNELDIR) M=$(CURDIR) modules
clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURDIR) clean

endif
