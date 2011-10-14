
WRS_BUILDROOT_DIR	= $(WRS_OUTPUT_DIR)/build/buildroot-2009.11
WRS_READLINE_DIR	= $(WRS_BUILDROOT_DIR)/output/build/readline-5.2
WRS_NCURSES_DIR		= $(WRS_BUILDROOT_DIR)/output/build/ncurses-5.6
NET_SNMP_HOME		= $(WRS_BUILDROOT_DIR)/output/build/net-snmp-5.5
NET_SNMP_CONFIG		= $(NET_SNMP_HOME)/net-snmp-config
WR_INSTALL_ROOT 	?= /usr/lib/white-rabbit
WR_LIB				= $(WR_INSTALL_ROOT)/lib


PROGRAM		= wrsw_cli
SRCFILES	= cli.c cli_commands.c cli_snmp.c
OBJFILES	= $(patsubst %.c,%.o,$(SRCFILES))
LIBFILES	= 	$(WRS_READLINE_DIR)/libreadline.a \
				$(WRS_READLINE_DIR)/libhistory.a \
				$(WRS_NCURSES_DIR)/lib/libncurses.a

CC		= $(CROSS_COMPILE)gcc

CFLAGS		= -O2 -Os -pipe -Wall -I. -I$(WRS_READLINE_DIR) \
	-I$(WRS_BUILDROOT_DIR)/output/build/net-snmp-5.5/include \
	-I$(WRS_BUILDROOT_DIR)/output/staging/usr/include \
	-I$(WRS_BUILDROOT_DIR)/output/staging/include \
	--sysroot=$(WRS_BUILDROOT_DIR)/output/staging/ \
	-isysroot $(WRS_BUILDROOT_DIR)/output/staging \
	-mtune=arm920t -march=armv4t -mabi=aapcs-linux -msoft-float \
	-D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -Ulinux \
	-Dlinux=linux -I.

LDFLAGS = -L. -L$(WR_LIB) -L$(WRS_BUILDROOT_DIR)/output/target/usr/lib \
          `$(NET_SNMP_CONFIG) --libs`



RM 	    := rm -f


.PHONY: all

all: $(PROGRAM)

$(PROGRAM): $(OBJFILES)
	$(CC) $(LDFLAGS) -o $(PROGRAM) $(OBJFILES) $(LIBFILES)

$(OBJFILES): %.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	$(RM) $(PROGRAM) $(OBJFILES)

install: all
	cp $(PROGRAM) $(WR_INSTALL_ROOT)/bin
