
SNMP_BUILD := $(wildcard $(WRS_OUTPUT_DIR)/build/buildroot-2*)
SNMP_BUILD := $(wildcard $(SNMP_BUILD)/output/build/netsnmp-*)

NET_SNMP_CONFIG = $(SNMP_BUILD)/net-snmp-config

CC      = $(CROSS_COMPILE)gcc
LD	= $(CROSS_COMPILE)ld

# defer running "net-snmp-config --cflags" so it is visible in make output
CFLAGS  += -fPIC -Wall  $$($(NET_SNMP_CONFIG) --cflags | sed s,-I/usr/include,,)
LDFLAGS = -shared $$($(NET_SNMP_CONFIG) --ldflags)
CFLAGS += -Iinclude -I../ppsi/include  -I../ppsi/arch-wrs/include -I../mini-rpc
CFLAGS += -DWRS_WITH_SNMP_HACKISH_LOG=0

SHLIB = wrsSnmp.so
SOURCES = init.c wrsScalar.c wrsPstats.c wrsPpsi.c wrsVersion.c
OBJECTS = $(SOURCES:.c=.o)

all: $(SHLIB)

$(SHLIB): $(OBJECTS)
	$(CC) $(LDFLAGS) $^ -o $@

clean:
	rm -f $(SHLIB) $(OBJECTS)

install:
	install -d  $(WR_INSTALL_ROOT)/lib
	install *.so $(WR_INSTALL_ROOT)/lib
