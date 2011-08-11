
PATCH_FILE   	= 0001-netsnmp-config-to-reduce-footprint.patch
PATCH_DIR   	= $(WRS_BASE_DIR)/../patches/buildroot
ROOTFS_OVERRIDE = $(WRS_BASE_DIR)/../userspace/rootfs_override

all:

# Get patch for the netsnmp config file used by buildroot. 
# netsnmp.mk.dst: original config file (netsnmp v.5.5). 
# netsnmp.mk.src: modified config file for reduced footprint.
# (Note patch directories are fixed using sed to set file paths relative
# to the buildrootdir variable used in build/scripts/wrs_build_buildroot)
patch:
	diff -u netsnmp.mk.dst netsnmp.mk.src \
        | sed -e '1 c --- /dev/null' -e '2 c +++ b/package/netsnmp/netsnmp.mk' \
        > $(PATCH_FILE)

clean:

# Install patch on patch dir
# Copy snmpd config file to the rootfs_override dir (create dir if necessary)
install:
	install $(PATCH_FILE) $(PATCH_DIR)
	install -d $(ROOTFS_OVERRIDE)/etc/snmp
	install snmpd.conf $(ROOTFS_OVERRIDE)/etc/snmp

