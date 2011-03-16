obj-m           := wr_rtu.o
KDIR            := ../../../kernel
PWD             := $(shell pwd)
# EXTRA_CFLAGS    := -I$(SUBDIRS)/../wr_vic
EXTRA_CFLAGS    :=
MAKE = make

default:
        $(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
