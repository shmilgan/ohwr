all:
	make -C ./src/mch_flasher
	make -C ./src/uboot_env
	make -C ./src/gen_fpga_image
	
clean:
	make -C ./src/mch_flasher clean
	make -C ./src/uboot_env clean
	make -C ./src/gen_fpga_image clean