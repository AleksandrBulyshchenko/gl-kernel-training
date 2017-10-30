obj-m := time.o

all:
	$(MAKE) -C $(BUILD_KERNEL) M=$(CURDIR) modules

clean:
	$(MAKE) -C $(BUILD_KERNEL) M=$(CURDIR) $@
