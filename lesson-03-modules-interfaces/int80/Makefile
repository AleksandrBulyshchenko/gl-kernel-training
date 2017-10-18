#
# syscalls implementation examples
#

ifneq ($(KERNELRELEASE),)

obj-m += mdu.o
obj-m += mdc.o

else

KERNELDIR := $(BUILD_KERNEL)
PROGS = mp mpsys mplib
CFLAGS := -m32 -static

.PHONY: all progs clean
all: progs
	$(MAKE) -C $(KERNELDIR) M=$(CURDIR) modules
progs: $(PROGS)
clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURDIR) clean
	rm -f $(PROGS)

endif
