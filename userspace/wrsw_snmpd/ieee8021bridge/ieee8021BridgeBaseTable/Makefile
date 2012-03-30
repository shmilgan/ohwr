
NET-SNMP-CONFIG = $(WRS_OUTPUT_DIR)/build/buildroot-2009.11/output/build/net-snmp-5.5/net-snmp-config
ROOTFS_OVERRIDE = $(WRS_BASE_DIR)/../userspace/rootfs_override

CC 	= $(CROSS_COMPILE)gcc

WR_INSTALL_ROOT ?= /usr/lib/white-rabbit
WR_INCLUDE = $(WR_INSTALL_ROOT)/include
WR_LIB = $(WR_INSTALL_ROOT)/lib

CFLAGS 	= -fPIC -I. -I../../../wrsw_hal -I../../../../../build/ptp-noposix/libwripc `$(NET-SNMP-CONFIG) --cflags`

OBJS =  ieee8021BridgeBaseTable_data_get.o \
	    ieee8021BridgeBaseTable_data_set.o \
	    ieee8021BridgeBaseTable_data_access.o \
	    ieee8021BridgeBaseTable.o \
	    ieee8021BridgeBaseTable_interface.o 


all:	$(OBJS)

$(OBJS): %.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@ 

clean:
	rm -f $(OBJS)

