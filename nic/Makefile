
obj-m := wr-nic.o
wr-nic-objs := module.o device.o nic-core.o endpoint.o ethtool.o \
		pps.o timestamp.o dmtd.o

# Try to force a -I from ../wr-headers. Note that this is re-run from $LINUX
WR_NIC_DIR ?= $(shell /bin/pwd)
export WR_NIC_DIR
EXTRA_CFLAGS := -I$(WR_NIC_DIR)/../wr-headers

# What follows is standard stuff
export ARCH ?= arm
export CROSS_COMPILE ?= $(CROSS_COMPILE_ARM)

all modules:
	$(MAKE) CONFIG_DEBUG_SECTION_MISMATCH=y \
		-C $(LINUX) SUBDIRS=$(shell /bin/pwd) modules

# We might "$(MAKE) -C $(LINUX)" but "make clean" with no LINUX defined
# is sometimes useful to have
clean:
	rm -f *.mod.c *.o *.ko .*cmd Module.symvers modules.order *~
	rm -rf .tmp_versions
