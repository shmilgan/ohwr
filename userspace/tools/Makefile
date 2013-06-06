TOOLS = rtu_stat wr_mon wr_phytool spll_dbg_proxy load-lm32 load-virtex com
TOOLS += mapper wmapper shw_ver

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

#
GIT_VER = $(shell git describe --always --dirty=+ | sed  's;^wr-switch-sw-;;')
GIT_USR = $(shell  git config --get-all user.name)

# LOTs of includes
CFLAGS =  -O2 -DDEBUG -g -Wall \
		-I$(LINUX)/include \
		-I$(LINUX)/arch/arm/mach-at91/include \
		-I../ptp-noposix/libptpnetif \
		-I../wrsw_hal \
		-I../wrsw_rtud \
		-I../mini-rpc \
		-I../ptp-noposix/PTPWRd \
		-I../include 

LDFLAGS =  -L$(WR_INSTALL_ROOT)/lib \
		-L../mini-rpc \
		-L../ptp-noposix \
		-L../libswitchhw \
		-lminipc -lptpnetif -lswitchhw

all: 	$(TOOLS)

rtu_stat: rtu_stat.o
	${CC} -o $@ $^ $(LDFLAGS)

wr_mon: wr_mon.o term.o
	${CC} -o $@ $^ $(LDFLAGS)

wr_phytool: wr_phytool.o ../wrsw_hal/rt_client.o
	${CC} -o $@ $^ $(LDFLAGS)

spll_dbg_proxy: spll_dbg_proxy.o
	${CC} -o $@ $^ $(LDFLAGS)

load-virtex: load-virtex.o load-fpga.o
	${CC} -o $@ $^ $(LDFLAGS)

load-lm32: load-lm32.o lm32-loader.o
	${CC} -o $@ $^ $(LDFLAGS)
	
shw_ver.o: shw_ver.c
	${CC} ${CFLAGS} -D__GIT_USR__="\"${GIT_USR}\"" -D__GIT_VER__="\"${GIT_VER}\"" $(LDFLAGS) -c -o $@ $^  
	
shw_ver: shw_ver.o
	${CC} -o $@ $^ $(LDFLAGS)	

clean:
	rm -f $(TOOLS) *.o *~

install: all
	install -d $(WR_INSTALL_ROOT)/bin
	install $(TOOLS) $(WR_INSTALL_ROOT)/bin
