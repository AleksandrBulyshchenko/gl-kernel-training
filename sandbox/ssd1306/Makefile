CONFIG_MODULE_SIG=n

obj-m := ssd1306.o

KDIR ?= ${HOME}/BBB/bb_kernel/
#KDIR ?= ~/BBB/KERNEL/

PWD := $(shell pwd)

default:
		  $(MAKE) ARCH=arm  -C $(KDIR) M=$(PWD) modules

clean:
		  $(MAKE) ARCH=arm  -C $(KDIR) SUBDIRS=$(PWD) clean

config:
		$(MAKE) ARCH=arm -C $(KDIR) menuconfig

dtb:
		$(MAKE) ARCH=arm -C $(KDIR) dtbs

.PHONY: clean default config dtb
