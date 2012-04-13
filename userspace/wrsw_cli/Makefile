
WRS_READLINE_DIR	= $(WRS_BUILDROOT_DIR)/output/build/readline-5.2
WRS_NCURSES_DIR		= $(WRS_BUILDROOT_DIR)/output/build/ncurses-5.6
WRS_NETSNMP_DIR		= $(WRS_BUILDROOT_DIR)/output/build/net-snmp-5.5
NET_SNMP_CONFIG		= $(WRS_NETSNMP_DIR)/net-snmp-config

# We must include some installed libraries. If this is built under build
# scripts, WR_INSTALL_ROOT = $WRS_OUTPUT_DIR/images/wr
WR_LIB 				= $(WR_INSTALL_ROOT)/lib

PROGRAM		= wrsw_cli
SRCFILES	= cli.c cli_commands.c cli_snmp.c cli_commands_utils.c \
				cmd_exit.c cmd_hostname.c cmd_mac_address_table.c \
				cmd_interface.c cmd_vlan.c cmd_mvrp.c \
				cmd_learning_constraints.c cmd_allocations.c
OBJFILES	= $(patsubst %.c,%.o,$(SRCFILES))
LIBFILES	= 	$(WRS_READLINE_DIR)/libreadline.a \
				$(WRS_READLINE_DIR)/libhistory.a \
				$(WRS_NCURSES_DIR)/lib/libncurses.a

CC		= $(CROSS_COMPILE)gcc

CFLAGS		= -O2 -Os -pipe -Wall -I. -I$(WRS_READLINE_DIR) \
			-I$(WRS_NETSNMP_DIR)/include -I../wrsw_rtud

LDFLAGS = -L. -L$(WR_LIB) -L$(WRS_BUILDROOT_DIR)/output/target/usr/lib \
          -lwripc -lminipc -lwrrtu `$(NET_SNMP_CONFIG) --libs`


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
