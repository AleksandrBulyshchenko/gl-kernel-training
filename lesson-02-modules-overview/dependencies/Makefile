#
# Linux modules dependencies
#

obj-m := dep_exporter.o dep_importer.o

dep_exporter-objs := exporter.o storage.o
dep_importer-objs := importer.o

all:
	$(MAKE) -C $(BUILD_KERNEL) M=$(CURDIR) modules

clean:
	$(MAKE) -C $(BUILD_KERNEL) M=$(CURDIR) clean
