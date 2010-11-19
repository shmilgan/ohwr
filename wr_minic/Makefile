

obj-m := wr_minic.o
KDIR := ../../../kernel
PWD := $(shell pwd)
EXTRA_CFLAGS := -I$(SUBDIRS)/../wr_vic
MAKE = make

default:
        $(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules