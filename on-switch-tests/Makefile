# This Makefile is based on wr-swithc-sw/userspace

# Most of the sub makefiles require some variables that used to be
# in the auto-generated ../Makedefs (when software was in svn).
# So let's add all of the needed variables here (environment can override)

LINUX ?= $(WRDEV_DIR)/software/rubi-repos/linux-wr
ARCH ?= arm
CROSS_COMPILE ?= $(CROSS_COMPILE_ARM_PATH)$(CROSS_COMPILE_ARM_PREFIX)
CROSS_COMPILE_ARM ?= $(CROSS_COMPILE)

# Installation of all of this stuff goes to images/wr in the output dir
WR_INSTALL_ROOT ?= $(WRS_OUTPUT_DIR)/images/wr
WRDEV_DIR ?= $(WRS_BASE_DIR)/..

# test directories need libs, so add them here (libswitchhw is in ./userspace)
CFLAGS = -I../libs -I../libs/libwripc -I../libs/libptpnetif \
	-I../../userspace/libswitchhw \
	-I../../userspace/wrsw_hal
LDFLAGS = -L../libs -lwripc -lptpnetif \
	-L../../userspace/libswitchhw -lswitchhw

# all variables are exported
export

# All targets must install as well, as later builds use headers/libs
all:
	$(MAKE) -C libs all
	$(MAKE) -C wr_mon all

clean:
	$(MAKE) -C libs clean
	$(MAKE) -C wr_mon clean

install: all
	$(MAKE) -C libs install
	$(MAKE) -C wr_mon install


