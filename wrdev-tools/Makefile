all:
	$(MAKE) -C ./src/mch_flasher
	$(MAKE) -C ./src/uboot_env
	$(MAKE) -C ./src/gen_fpga_image

clean:
	$(MAKE) -C ./src/mch_flasher clean
	$(MAKE) -C ./src/uboot_env clean
	$(MAKE) -C ./src/gen_fpga_image clean