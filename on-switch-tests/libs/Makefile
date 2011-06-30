# Makefile based on ptp-noposix, whence libs have been lifted

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

# Flags for standalone compilation
TOPDIR=$(shell /bin/pwd)
CFLAGS += -Wall -ggdb -I$(TOPDIR)/wrsw_hal -I$(TOPDIR)/libwripc \
	-I$(TOPDIR)/libptpnetif -I$(TOPDIR)/PTPWRd -I$(LINUX)/include \
	-include compat.h -include ptpd-wrappers.h

# These are lifted in the ptp.o temporary object file, for me to see the size
CORELIBS = libwripc.a libptpnetif.a


all libs: check $(CORELIBS)

# we only support cross-compilation (if you want force CROSS_COMPILE to " ")
# similarly, we need a kernel at this time
CHECKVARS = LINUX

check:
	@for n in $(CHECKVARS); do \
	    if [ -z "$$(eval echo \$$$$n)" ]; then \
	        echo "Please set $$n" >& 2; exit 1; \
	    fi \
	done

libwripc.a: libwripc/wr_ipc.o libwripc/helper_arm.o
	$(AR) r $@ $^

libptpnetif.a: libptpnetif/hal_client.o libptpnetif/ptpd_netif.o
	$(AR) r $@ $^

# clean and so on.
clean:
	rm -f *.a */*.o */dep/*.o *~ */*~

# even if there are files with these names, ignore them
.PHONY: all check libs clean
