
obj-m = at91_softpwm.o 
at91_softpwm-objs := fiq-asm.o fiq-at91sam92.o fiq-module.o softpwm_core.o

export ARCH ?= arm
export CROSS_COMPILE ?= $(CROSS_COMPILE_ARM)

all modules:
	$(MAKE) CONFIG_DEBUG_SECTION_MISMATCH=y \
		-C $(LINUX) SUBDIRS=$(shell /bin/pwd) modules

# We might "$(MAKE) -C $(LINUX)" but "make clean" with no LINUX defined
# is sometimes useful to have
clean:
	rm -f *.mod.c *.o *.ko .*cmd Module.symvers modules.order
	rm -rf .tmp_versions

