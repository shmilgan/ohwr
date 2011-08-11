NET_SNMP_HOME	= $(WRS_OUTPUT_DIR)/build/buildroot-2009.11/output/build/net-snmp-5.5
NET-SNMP-CONFIG = $(NET_SNMP_HOME)/net-snmp-config
ROOTFS_OVERRIDE = $(WRS_BASE_DIR)/../userspace/rootfs_override

CC 	= $(CROSS_COMPILE)gcc

CFLAGS 	= -fPIC -I. `$(NET-SNMP-CONFIG) --cflags` -I../../wrsw_rtud
LDFLAGS = -fPIC -shared `$(NET-SNMP-CONFIG) --ldflags`

OBJS 	= \
ieee8021QBridgeTable.o \
ieee8021QBridgeFdbTable.o \
ieee8021QBridgeStaticUnicastTable.o \
ieee8021QBridgeStaticMulticastTable.o \
ieee8021QBridgeTpFdbTable.o \
ieee8021QBridgeTpGroupTable.o 
#ieee8021QBridgeNextFreeLocalVlanTable.o \
#ieee8021QBridgePortVlanTable.o \
#ieee8021QBridgeVlanCurrentTable.o \
#ieee8021QBridgeVlanNumDeletes.o \
#ieee8021QBridgeVlanStaticTable.o 

LIB 	= libieee8021qbridge.so


all:	$(LIB)

$(LIB): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

install: all
	install -d $(ROOTFS_OVERRIDE)/usr/lib
	install $(LIB) $(ROOTFS_OVERRIDE)/usr/lib

clean:
	rm -f $(LIB) $(OBJS)

