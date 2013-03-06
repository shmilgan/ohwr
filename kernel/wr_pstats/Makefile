obj-m		:= wr_pstats.o
LINUX		?= ../../../kernel

export ARCH ?= arm
export CROSS_COMPILE ?= $(CROSS_COMPILE_ARM)

all modules:
	$(MAKE) -C $(LINUX) SUBDIRS=$(shell /bin/pwd) modules

clean:
	rm -f *.mod.c *.o *.ko *.i .*cmd Module.symvers modules.order *~
	rm -rf .tmp_versions
