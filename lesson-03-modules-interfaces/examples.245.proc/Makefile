KDIR = ${BUILD_KERNEL}
PWD = $(shell pwd)
CC = gcc -Wall

EXTRA_CFLAGS += -std=gnu99

TARGET1 = mod_procr
TARGET2 = mod_procr2
TARGET3 = mod_proc
TARGET4 = mod_proct
TARGET5 = mod_2

obj-m  := $(TARGET1).o $(TARGET2).o $(TARGET3).o $(TARGET4).o $(TARGET5).o

all: default mcat clean

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

mcat:  common.h mcat.c
	gcc -m32 -static -I$(KDIR) mcat.c -o mcat

clean:
	@rm -f *.o .*.cmd .*.flags *.mod.c *.order
	@rm -f .*.*.cmd *.symvers *~ *.*~
	@rm -fR .tmp*
	@rm -rf .tmp_versions

disclean: clean
	@rm -f *.ko mcat
