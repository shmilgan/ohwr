
PROGRAM		= wrsw_gps
SRCFILES	= wrsw_gps.c
OBJFILES	= $(patsubst %.c,%.o,$(SRCFILES))

CC		= $(CROSS_COMPILE)gcc

# We must include stuff from various headers, which are installed.
# If this is build under build scripts, it's $WRS_OUTPUT_DIR/images/wr
WR_INSTALL_ROOT ?= /usr/lib/white-rabbit
WR_INCLUDE = $(WR_INSTALL_ROOT)/include
WR_LIB = $(WR_INSTALL_ROOT)/lib

CFLAGS		= -O2 -DDEBUG -Wall -ggdb  \
		      -I../include -I$(WR_INCLUDE) -I$(LINUX)/include

LDFLAGS 	:= -L$(WR_LIB)
RM 	    := rm -f

.PHONY: all

all: $(PROGRAM)

$(PROGRAM): $(OBJFILES)
	$(CC) -o $@ $(OBJFILES) $(LDFLAGS) 

clean:
	$(RM) $(PROGRAM) $(OBJFILES)

install: all
	cp $(PROGRAM) $(WR_INSTALL_ROOT)/bin
