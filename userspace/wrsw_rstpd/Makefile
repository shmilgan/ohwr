
PROGRAM		= wrsw_rstpd
SRCFILES	= rstp_main.c rstp_epoll_loop.c rstp_frame.c utils.c rstp_if.c \
			rstp_data.c rstp_stmch.c
OBJFILES	= $(patsubst %.c,%.o,$(SRCFILES))

CC		= $(CROSS_COMPILE)gcc

WR_INSTALL_ROOT ?= /usr/lib/white-rabbit
WR_INCLUDE = $(WR_INSTALL_ROOT)/include
WR_LIB = $(WR_INSTALL_ROOT)/lib

CFLAGS		= -O2 -DDEBUG -Wall -ggdb -DTRACE_ALL -I. -I../include -I../wrsw_hal \
			-I$(WRS_OUTPUT_DIR)/build/ptp-noposix/libwripc
LDFLAGS 	:= -L$(WR_LIB) -lswitchhw -lwripc

RM 	    := rm -f


.PHONY: all

all: $(PROGRAM)

$(PROGRAM): $(OBJFILES)
	$(CC) -o $@ $(OBJFILES) $(LDFLAGS)

clean:
	$(RM) $(PROGRAM) $(OBJFILES)

install: all
	cp $(PROGRAM) $(WR_INSTALL_ROOT)/bin
