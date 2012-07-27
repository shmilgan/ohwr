
# Trivial makefile for 2.6 modules

#LINUX ?= /lib/modules/$(shell uname -r)/build

obj-m = at91_softpwm.o 

at91_softpwm-objs := fiq-asm.o fiq-at91sam92.o fiq-module.o softpwm_core.o

all:
	$(MAKE) -C $(LINUX) M=$(shell /bin/pwd) modules

clean:
	$(MAKE) -C $(LINUX) clean M=$(shell /bin/pwd)
	rm -f Module.symvers modules.order *~
#	$(MAKE) -C doc terse
