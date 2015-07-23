WR_INSTALL_ROOT ?= /usr/lib/white-rabbit

# # Standard stanza for cross-compilation (courtesy of the linux makefile)

AS              = $(CROSS_COMPILE)as
LD              = $(CROSS_COMPILE)ld
CC              = $(CROSS_COMPILE)gcc
CPP             = $(CC) -E
AR              = $(CROSS_COMPILE)ar
NM              = $(CROSS_COMPILE)nm
STRIP           = $(CROSS_COMPILE)strip
OBJCOPY         = $(CROSS_COMPILE)objcopy
OBJDUMP         = $(CROSS_COMPILE)objdump

CFLAGS =  -D KBUILD_NO_NLS

# most of this is just copying stuff in
RFILES = Kconfig dot-config wrs_release_defconfig
XFILES = conf
FILE = $(RFILES) $(XFILES)

all: $(FILES)

install: all
	install -d $(WR_INSTALL_ROOT)/etc
	install -m 644 $(RFILES) $(WR_INSTALL_ROOT)/etc
	install -m 755 $(XFILES) $(WR_INSTALL_ROOT)/etc
	install -m 644 Makefile-for-target $(WR_INSTALL_ROOT)/etc/Makefile

clean:
	rm -f $(FILES)

dot-config: $(WRS_BASE_DIR)/../.config
	cp $^ $@

Kconfig: $(WRS_BASE_DIR)/../Kconfig
	cp $^ $@

conf:
	$(CC) $(CFLAGS) -o $@ $(WRS_BASE_DIR)/../scripts/kconfig/conf.c \
		$(WRS_BASE_DIR)/../scripts/kconfig/zconf.tab.c

wrs_release_defconfig: $(WRS_BASE_DIR)/../configs/wrs_release_defconfig
	cp $^ $@

