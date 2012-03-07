/* Userspace /dev/mem I/O functions for accessing the Main FPGA */

#include <stdio.h>
#include <unistd.h>

#include <unistd.h>
#include <sys/mman.h>
#include <inttypes.h>
#include <fcntl.h>
#include <poll.h>

#include <hw/switch_hw.h>

#define SMC_CS0_BASE 0x10000000
#define SMC_CS0_SIZE 0x200000

#define SPI_CLKDIV_VAL 20 // clock divider for CMI SPI bus clock

/* Virtual base address of the Main FPGA address space. */
volatile uint8_t *_fpga_base_virt;

/* Initializes the mapping of the Main FPGA to the CPU address space. */
int shw_fpga_mmap_init()
{
  int fd;

    TRACE(TRACE_INFO, "Initializing FPGA memory mapping.");

    fd = open("/dev/mem", O_RDWR | O_SYNC);
  if(!fd) return -1;
  _fpga_base_virt = mmap(NULL, SMC_CS0_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, SMC_CS0_BASE);

  if(_fpga_base_virt == NULL)
  {
    perror("mmap()");
    close(fd);
    return -1;
  }

    TRACE(TRACE_INFO, "FPGA virtual base = 0x%08x", _fpga_base_virt);

    return 0;

}
