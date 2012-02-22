
PROGRAM		= wrsw_rtud
SRCFILES	= mac.c rtu_drv.c rtu_hash.c rtu_sw.c rtu_fd.c \
              rtu_fd_srv.c endpoint_hw.c rtud.c utils.c
OBJFILES	= $(patsubst %.c,%.o,$(SRCFILES))

CC		    = $(CROSS_COMPILE)gcc

# We must include stuff from various headers, which are installed.
# If this is build under build scripts, it's $WRS_OUTPUT_DIR/images/wr
WR_INSTALL_ROOT ?= /usr/lib/white-rabbit
WR_INCLUDE = $(WR_INSTALL_ROOT)/include
WR_LIB = $(WR_INSTALL_ROOT)/lib

# Use -DV3 with v3 HW (sets default PVID = 1 and checks for reserved VIDs)
CFLAGS		= -O2 -DDEBUG -Wall -ggdb -DTRACE_ALL \
			-I. -I../include -I$(WR_INCLUDE) -I$(LINUX)/include # -DV3
# -I$(CROSS_COMPILE_ARM_PATH)/../include

LDFLAGS 	:= -L. -L$(WR_LIB) \
               -lswitchhw -lptpnetif -lminipc -lwripc -lpthread

RM 	    := rm -f

all: $(PROGRAM) $(LIB)

$(PROGRAM): $(OBJFILES)
	$(CC) -o $@ $(OBJFILES) $(LDFLAGS)

clean:
	$(RM) $(PROGRAM) $(OBJFILES)

install: all
	cp $(PROGRAM) $(WR_INSTALL_ROOT)/bin
