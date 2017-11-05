#
# Problem with building multiple file modules. When I uncomment lines below,
# module will be built succesfully.  Installing and removing module will be
# without any warnings or errors,  but classes will  not  be  created,  any
# prints will not be in the dmesg.
#
#  Used BOOK_PRACTIC_245 p. 58
#

ifneq ($(KERNELRELEASE),)

TARGET = usrmod

#OBJS = fib.o

obj-m += ${TARGET}.o
#$(TARGET)-objs := $(OBJS)

else

KERNELDIR := $(BUILD_KERNEL)

.PHONY: all clean
all:
	$(MAKE) -C $(KERNELDIR) M=$(CURDIR) modules

#$(TARGET).o: $(OBJS)
#	$(LD) -r -o $@ $(OBJS)

clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURDIR) clean

endif
