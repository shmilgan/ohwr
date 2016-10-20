
export ARCH ?= arm
export CROSS_COMPILE ?= $(CROSS_COMPILE_ARM)

all: modules
modules:
	$(MAKE) -C $(LINUX) M=$(shell /bin/pwd)
clean:
	$(MAKE) -C $(LINUX) M=$(shell /bin/pwd) clean
gtags:
	gtags --statistics

.PHONY: all clean gtags
