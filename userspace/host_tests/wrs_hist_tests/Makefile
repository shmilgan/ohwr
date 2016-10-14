vpath %.c ../../wrs_hist

OBJS = hist_sfp.o hist_main.o hist_crc.o hist_uptime.o hist_sfp_test.o

BINARY = hist_sfp_test
# this is a host version of libwr, ignore CROSS_COMPILE
CROSS_COMPILE:=
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

CFLAGS = -Wall -I. -O2 -ggdb \
	-Wstrict-prototypes \
	-DLIBWR_INTERNAL \
	-I../../libwr/include \
	-I../../include \
	-I../../wrs_hist

GIT_VER = $(shell git describe --always --dirty | sed  's;^wr-switch-sw-;;')
CFLAGS += -D__GIT_VER__="\"${GIT_VER}\""

LDFLAGS = -L../../host_tools/libwr -lwr -lrt

all: $(BINARY)

hist_sfp_test: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(BINARY) $(OBJS)

cleanall: clean
	rm -f wrs-shmem-*
