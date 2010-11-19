obj-m := wr_vic.o
KDIR := ../../../kernel
PWD := $(shell pwd)
MAKE = make

default:
        $(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
