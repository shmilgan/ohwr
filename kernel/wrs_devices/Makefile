LINUX ?= /lib/modules/$(shell uname -r)/build

all: modules
modules:
	$(MAKE) -C $(LINUX) M=$(shell /bin/pwd)
clean:
	$(MAKE) -C $(LINUX) M=$(shell /bin/pwd) clean
gtags:
	gtags --statistics

.PHONY: all clean gtags
