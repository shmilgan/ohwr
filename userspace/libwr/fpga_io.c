/* Userspace /dev/mem I/O functions for accessing the Main FPGA */

#include <stdio.h>
#include <unistd.h>

#include <unistd.h>
#include <sys/mman.h>
#include <inttypes.h>
#include <fcntl.h>
#include <poll.h>

#include <libwr/switch_hw.h>
#include <libwr/wrs-msg.h>

#include <fpga_io.h>

#define SMC_CS0_BASE 0x10000000
#define SMC_CS0_SIZE 0x200000

/* Virtual base address of the Main FPGA address space. */
volatile uint8_t *_fpga_base_virt;

/* Initializes the mapping of the Main FPGA to the CPU address space. */
int shw_fpga_mmap_init()
{
	int fd;

	pr_debug("Initializing FPGA memory mapping.\n");

	fd = open("/dev/mem", O_RDWR | O_SYNC);
	if (fd < 0) {
		perror("/dev/mem");
		return -1;
	}
	_fpga_base_virt =
	    mmap(NULL, SMC_CS0_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
		 SMC_CS0_BASE);
	close(fd);

	if (_fpga_base_virt == MAP_FAILED) {
		perror("mmap()");
		return -1;
	}

	pr_debug("FPGA virtual base = %p\n", _fpga_base_virt);

	return 0;

}
