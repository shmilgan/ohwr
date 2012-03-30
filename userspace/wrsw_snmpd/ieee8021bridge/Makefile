
NET-SNMP-CONFIG = $(WRS_OUTPUT_DIR)/build/buildroot-2009.11/output/build/net-snmp-5.5/net-snmp-config
ROOTFS_OVERRIDE = $(WRS_BASE_DIR)/../userspace/rootfs_override

IEEE8021BRIDGEBASETABLE = ieee8021BridgeBaseTable/ieee8021BridgeBaseTable
IEEE8021BRIDGEBASEPORTTABLE = ieee8021BridgeBasePortTable/ieee8021BridgeBasePortTable
IEEE8021BRIDGETPPORTTABLE = ieee8021BridgeTpPortTable/ieee8021BridgeTpPortTable
IEEE8021BRIDGEPORTPRIORITYTABLE = ieee8021BridgePortPriorityTable/ieee8021BridgePortPriorityTable
IEEE8021BRIDGETRAFFICCLASSTABLE = ieee8021BridgeTrafficClassTable/ieee8021BridgeTrafficClassTable

CC 	= $(CROSS_COMPILE)gcc

WR_INSTALL_ROOT ?= /usr/lib/white-rabbit
WR_INCLUDE = $(WR_INSTALL_ROOT)/include
WR_LIB = $(WR_INSTALL_ROOT)/lib

LDFLAGS = -fPIC -shared -L. -lwripc `$(NET-SNMP-CONFIG) --ldflags`


OBJS =  $(IEEE8021BRIDGEBASETABLE)_data_get.o \
	    $(IEEE8021BRIDGEBASETABLE)_data_set.o \
	    $(IEEE8021BRIDGEBASETABLE)_data_access.o \
	    $(IEEE8021BRIDGEBASETABLE).o \
	    $(IEEE8021BRIDGEBASETABLE)_interface.o 

OBJS += $(IEEE8021BRIDGEBASEPORTTABLE)_data_get.o \
	    $(IEEE8021BRIDGEBASEPORTTABLE)_data_set.o \
	    $(IEEE8021BRIDGEBASEPORTTABLE)_data_access.o \
	    $(IEEE8021BRIDGEBASEPORTTABLE).o \
	    $(IEEE8021BRIDGEBASEPORTTABLE)_interface.o

OBJS += $(IEEE8021BRIDGETPPORTTABLE)_data_get.o \
	    $(IEEE8021BRIDGETPPORTTABLE)_data_set.o \
	    $(IEEE8021BRIDGETPPORTTABLE)_data_access.o \
	    $(IEEE8021BRIDGETPPORTTABLE).o \
	    $(IEEE8021BRIDGETPPORTTABLE)_interface.o

OBJS += $(IEEE8021BRIDGEPORTPRIORITYTABLE)_data_get.o \
	    $(IEEE8021BRIDGEPORTPRIORITYTABLE)_data_set.o \
	    $(IEEE8021BRIDGEPORTPRIORITYTABLE)_data_access.o \
	    $(IEEE8021BRIDGEPORTPRIORITYTABLE).o \
	    $(IEEE8021BRIDGEPORTPRIORITYTABLE)_interface.o

OBJS += $(IEEE8021BRIDGETRAFFICCLASSTABLE)_data_get.o \
	    $(IEEE8021BRIDGETRAFFICCLASSTABLE)_data_set.o \
	    $(IEEE8021BRIDGETRAFFICCLASSTABLE)_data_access.o \
	    $(IEEE8021BRIDGETRAFFICCLASSTABLE).o \
	    $(IEEE8021BRIDGETRAFFICCLASSTABLE)_interface.o

LIB 	= libieee8021bridge.so


all:	
	cd ieee8021BridgeBaseTable; make -f Makefile;
	cd ieee8021BridgeBasePortTable; make -f Makefile;
	cd ieee8021BridgeTpPortTable; make -f Makefile;
	cd ieee8021BridgePortPriorityTable; make -f Makefile;
	cd ieee8021BridgeTrafficClassTable; make -f Makefile;
	$(CC) -o $(LIB) $(LDFLAGS) $(OBJS)


install: all
	install -d $(ROOTFS_OVERRIDE)/usr/lib
	install $(LIB) $(ROOTFS_OVERRIDE)/usr/lib
	install libwripc.so $(ROOTFS_OVERRIDE)/usr/lib

clean:
	rm -f $(LIB) $(OBJS)

