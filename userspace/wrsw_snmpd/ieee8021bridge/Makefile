
NET-SNMP-CONFIG = $(WRS_BUILDROOT_DIR)/output/build/net-snmp-5.5/net-snmp-config
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
	$(MAKE) -C ieee8021BridgeBaseTable all
	$(MAKE) -C ieee8021BridgeBasePortTable all
	$(MAKE) -C ieee8021BridgeTpPortTable all
	$(MAKE) -C ieee8021BridgePortPriorityTable all
	$(MAKE) -C ieee8021BridgeTrafficClassTable all
	$(CC) -o $(LIB) $(LDFLAGS) $(OBJS)


install: all
	install -d $(ROOTFS_OVERRIDE)/usr/lib
	install $(LIB) $(ROOTFS_OVERRIDE)/usr/lib
	install libwripc.so $(ROOTFS_OVERRIDE)/usr/lib

clean:
	$(MAKE) -C ieee8021BridgeBaseTable clean
	$(MAKE) -C ieee8021BridgeBasePortTable clean
	$(MAKE) -C ieee8021BridgeTpPortTable clean
	$(MAKE) -C ieee8021BridgePortPriorityTable clean
	$(MAKE) -C ieee8021BridgeTrafficClassTable clean

