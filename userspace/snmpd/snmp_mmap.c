#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include "snmp_mmap.h"

/* FIXME: this is copied from wr_date, should be librarized */
void *create_map(unsigned long address, unsigned long size)
{
	unsigned long ps = getpagesize();
	unsigned long offset, fragment, len;
	void *mapaddr;
	int fd;

	fd = open("/dev/mem", O_RDWR | O_SYNC);
	if (fd < 0)
		return NULL;

	offset = address & ~(ps - 1);
	fragment = address & (ps - 1);
	len = address + size - offset;

	mapaddr = mmap(0, len, PROT_READ | PROT_WRITE,
		       MAP_SHARED, fd, offset);
	close(fd);
	if (mapaddr == MAP_FAILED)
		return NULL;
	return mapaddr + fragment;
}
