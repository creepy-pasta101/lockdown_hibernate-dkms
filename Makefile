obj-m := lockdown_hibernate.o

ccflags-y := -Os -g -ggdb

KERNELRELEASE ?= $(shell uname -r)
KDIR ?= /lib/modules/$(KERNELRELEASE)/build

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
