
SNMP_BUILD := $(wildcard $(WRS_OUTPUT_DIR)/build/buildroot-2*)
SNMP_BUILD := $(wildcard $(SNMP_BUILD)/output/build/netsnmp-*)

NET_SNMP_CONFIG = $(SNMP_BUILD)/net-snmp-config

CC      = $(CROSS_COMPILE)gcc
LD	= $(CROSS_COMPILE)ld

# defer running "net-snmp-config --cflags" so it is visible in make output
CFLAGS  = -fPIC   $$($(NET_SNMP_CONFIG) --cflags)
LDFLAGS = -shared $$($(NET_SNMP_CONFIG) --ldflags)

SHLIB = wrsSnmp.so
SOURCES = init.c wrsScalar.c
OBJECTS = $(SOURCES:.c=.o)

all: $(SHLIB)

$(SHLIB): $(OBJECTS)
	$(CC) $(LDFLAGS) $^ -o $@

clean:
	rm -f $(SHLIB) $(OBJECTS)

install:
	install -d  $(WR_INSTALL_ROOT)/lib
	install *.so $(WR_INSTALL_ROOT)/lib
