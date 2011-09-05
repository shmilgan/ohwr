
PROGRAM		= wrsw_rtud
SRCFILES	= mac.c rtu_drv.c rtu_hash.c rtu_sw.c rtu_fd.c rtu_fd_proxy.c \
              rtu_fd_srv.c rtud.c utils.c
OBJFILES	= $(patsubst %.c,%.o,$(SRCFILES))

CC		= $(CROSS_COMPILE)gcc

# We must include stuff from various headers, which are installed.
# If this is build under build scripts, it's $WRS_OUTPUT_DIR/images/wr
WR_INSTALL_ROOT ?= /usr/lib/white-rabbit
WR_INCLUDE = $(WR_INSTALL_ROOT)/include
WR_LIB = $(WR_INSTALL_ROOT)/lib

CFLAGS		= -O2 -DDEBUG -Wall -ggdb -DTRACE_ALL \
			-I. -I../include -I$(WR_INCLUDE) -I$(LINUX)/include
# -I$(CROSS_COMPILE_ARM_PATH)/../include

LDFLAGS 	:= -L$(WR_LIB) -lminipc -lswitchhw -lptpnetif  -lwripc -lpthread

RM 	    := rm -f


.PHONY: all

all: $(PROGRAM)

$(PROGRAM): $(OBJFILES)
	$(CC) -o $@ $(OBJFILES) $(LDFLAGS)

clean:
	$(RM) $(PROGRAM) $(OBJFILES)

install: all
	cp $(PROGRAM) $(WR_INSTALL_ROOT)/bin
