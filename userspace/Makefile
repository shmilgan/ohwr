include ../Makedefs

all:
			make -C 3rdparty all
			make -C rubi-repos/wr-switch-software LINUX=$(WRDEV_DIR)/software/rubi-repos/linux-wr ARCH=arm CROSS_COMPILE_ARM=$(CROSS_COMPILE_ARM_PATH)$(CROSS_COMPILE_ARM_PREFIX) all
			make -C rubi-repos/ptp-noposix LINUX=$(WRDEV_DIR)/software/rubi-repos/linux-wr ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE_ARM_PATH)$(CROSS_COMPILE_ARM_PREFIX) all
			make -C libswitchhw all
			make -C wrsw_hal all
			make -C wrsw_rtud all
			make -C tests/rtu_stat TARGET=ARM all

clean:
			rm -r -f $(WR_INSTALL_ROOT)/lib
			rm -r -f $(WR_INSTALL_ROOT)/bin
			rm -r -f $(WR_INSTALL_ROOT)/firmware
			make -C rubi-repos/wr-switch-software clean
			make -C rubi-repos/ptp-noposix clean	
			make -C 3rdparty clean
			make -C libswitchhw clean
			make -C wrsw_hal clean
			make -C wrsw_rtud clean
#			make -C tests/wr_mon clean
			make -C tests/rtu_stat clean

deploy: all
			make -C 3rdparty deploy
			make -C libswitchhw deploy
			make -C wrsw_hal deploy
			make -C wrsw_rtud deploy
			make -C tests/rtu_stat TARGET=ARM deploy
			./rubi-repos/deploy.sh $(WR_INSTALL_ROOT)
			
rootfs: deploy
			mkdir -p $(WR_INSTALL_ROOT)/firmware
			cp $(WRDEV_DIR)/hdl/bin/*.bin $(WR_INSTALL_ROOT)/firmware		 
			../scripts/build_rootfs.sh