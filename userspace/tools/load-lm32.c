/* 
 * Copyright (c) 2011 Grzegorz Daniluk <g.daniluk@elproma.com.pl>
 * ELF support added by Alessandro Rubini for CERN, 2014
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <elf.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <arpa/inet.h> /* htonl */

#define BASE_FPGA 0x10000000
#define SIZE_FPGA 0x20000


#define LM32_RAM_BASE 0x0
#define GPIO_BASE 0x10300
#define GPIO_COR  0x0
#define GPIO_SOR  0x4
#define LM32_RESET_PIN 2

static void *base_fpga;
static char *prgname;


static void fpga_writel(uint32_t data, uint32_t addr)
{
	*(volatile uint32_t *)(base_fpga + addr) = data;
}

static uint32_t fpga_readl(uint32_t addr)
{
	return	*(volatile uint32_t *)(base_fpga + addr);
}

/* The original "conv_endian" was bound to 32 bits. This is any-size */
#define BE(datum)					\
({	     __typeof__(datum) result;				\
	     switch(sizeof(datum)) {				\
	     case 1: result = (datum); break;			\
	     case 2: result = htons((datum)); break;		\
	     case 4: result = htonl((datum)); break;		\
	     default: kill(getpid(), SIGUSR1);			\
	     }							\
	     result;						\
})

static void rst_lm32(int rst)
{
	fpga_writel(1 << LM32_RESET_PIN,
		    GPIO_BASE + (rst ? GPIO_SOR : GPIO_COR));
}

static int copy_lm32(void *data, int noload, int size, uint32_t base_addr)
{
  int i;
  uint32_t *buf = data; /* be 32-bit oriented in writing */
  int buf_nwords = (size + 3) / 4;

  /* Do not actually load anything. This is used to read/write variables */
  if (noload)
	  return 0;

  printf("Writing memory (0x%04x bytes at 0x%04x): ", size, base_addr);

  for(i=0;i<buf_nwords;i++)
  {
	fpga_writel(BE(buf[i]), base_addr + i *4);
	if(!(i & 0xfff))
		printf(".");
  }

  printf("\nVerifing memory: ");

  for(i=0;i<buf_nwords;i++)
  {
	uint32_t x = fpga_readl(base_addr+ i*4);
	if(BE(buf[i]) != x)
	{
		printf("Verify failed (%x vs %x)\n", BE(buf[i]), x);
		return -1;
	}

	if(!(i & 0xfff))
		printf(".");
  }
  printf(" OK.\n");
  return 0;
}

static char *global_strptr;
static Elf32_Shdr *global_sh;

/* The elf loader relies on the binary loader above (and the global mmap) */
static int copy_lm32_elf(void *data, int noload, int size)
{
	int i, flags, verbose = getenv("LOAD_LM32_VERBOSE") != NULL;
	Elf32_Ehdr *eh;
	Elf32_Phdr *ph;
	Elf32_Shdr *sh;
	char *strptr;

	eh = data;
	if (verbose) {
		printf("type:    %8i\n", BE(eh->e_type));
		printf("machine: %8i\n", BE(eh->e_machine));
		printf("version: %8i\n", BE(eh->e_version));
		printf("entry:   %08lx\n", (long)BE(eh->e_entry));
		printf("phoff:   %8i\n", BE(eh->e_phoff));
		printf("shoff:   %8i\n", BE(eh->e_shoff));
		printf("ehsize:  %8i\n", BE(eh->e_ehsize));
		printf("shstrndx:%8i\n", BE(eh->e_shstrndx));
	}

	ph = (Elf32_Phdr *)((char *)eh + (int)(BE(eh->e_phoff)));

	/* program headers. Irrelevant, actually... */
	for (i = 0; i < BE(eh->e_phnum); i++) {
		flags = BE(ph->p_flags);
		if (verbose) {
			printf("prg: %i 0x%08lx, 0x%08lx, 0x%08lx, %c%c%c "
			       "(%08x), %i, %i %i\n",
			       BE(ph->p_type),
			       (long)(BE(ph->p_offset)),
			       (long)(BE(ph->p_vaddr)),
			       (long)(BE(ph->p_paddr)),
			       flags & PF_R ? 'r' : '-',
			       flags & PF_W ? 'w' : '-',
			       flags & PF_X ? 'x' : '-',
			       flags,
			       BE(ph->p_filesz),
			       BE(ph->p_memsz),
			       BE(ph->p_align));
		}
	}

	/* first loop: look for strtab */
	sh = (Elf32_Shdr *)((char *)eh + (int)(BE(eh->e_shoff)));
	for (i = 0; i < BE(eh->e_shstrndx); i++)
		sh=(Elf32_Shdr *)((char *)sh + (int)BE(eh->e_shentsize));
	strptr = (char *)eh + BE(sh->sh_offset);
	sh = (Elf32_Shdr *)((char *)eh + (int)(BE(eh->e_shoff)));

	/* Save them for later (setting vriables) */
	global_strptr = strptr;
	global_sh = sh;

	/* Section headers: this is what we load */
	for (i = 0; i < BE(eh->e_shnum); i++) {
		unsigned long off, len, ram;
		if (i) /* next header */
			sh = (Elf32_Shdr *)((char *)sh
					    + (int)BE(eh->e_shentsize));

		if (verbose) {
		printf("sect: %3i %-25.25s %2i 0x%08lx, 0x%08lx, (%i) %i %i\n",
		       BE(sh->sh_name),
		       strptr + BE(sh->sh_name),
		       BE(sh->sh_type),
		       (long)(BE(sh->sh_offset)),
		       (long)(BE(sh->sh_addr)),
		       BE(sh->sh_size),
		       BE(sh->sh_addralign),
		       BE(sh->sh_entsize));
		}
		off = BE(sh->sh_offset);
		len = BE(sh->sh_size);
		ram = BE(sh->sh_addr) & 0x0fffffff;

		/* ignore unloadable sections */
		if (BE(sh->sh_type) != SHT_PROGBITS)
			continue;
		if (!(BE(sh->sh_flags) & SHF_ALLOC))
			continue;
		if (len == 0)
			continue;

		/*
		 * First argument is base in file, third is offset
		 * in both fpga and file, so adjust file base (hack)
		 */
		if (copy_lm32(data + off, noload, len, ram))
			return -1;
	}
	return 0;
}



int load_lm32(char *fname, int noload)
{
	void *buf;
	FILE *f;
	int fdmem, iself, ret;

	setbuffer(stdout, NULL, 0);
	if ((fdmem = open("/dev/mem", O_RDWR | O_SYNC)) < 0) {
		fprintf(stderr, "%s: /dev/mem: %s\n", prgname,
			strerror(errno));
		exit(1);
	}

	base_fpga = mmap(0, SIZE_FPGA, PROT_READ | PROT_WRITE,
		       MAP_SHARED, fdmem,
		       BASE_FPGA);
	close(fdmem);

	if (base_fpga == MAP_FAILED) {
		fprintf(stderr, "%s: mmap(/dev/mem): %s\n",
			prgname, strerror(errno));
		exit(1);
	}


	f=fopen(fname,"rb");
	if(!f)
	{
		fprintf(stderr, "%s: %s: %s\n", prgname,
			fname, strerror(errno));
		return -1;
	}

	fseek(f, 0, SEEK_END);
	int size = ftell(f);
	rewind(f);

	buf = malloc(size + 4);
	ret = fread(buf, 1, size, f);
	fclose(f);
	if (ret != size) {
		fprintf(stderr, "%s: %s: read error (\n", prgname, fname);
		return -1;
	}

	if (!memcmp(buf, ELFMAG, SELFMAG))
		iself = 1;
	else if (!memcmp(buf, "\x98\0\0\0", 4))
		iself = 0;
	else {
		fprintf(stderr, "%s: %s: Unrecognized file type\n", prgname,
			fname);
		return -1;
	}

	if (!noload)
		rst_lm32(1);

	/*
	 * If else, we need to call the function even if (noload)
	 * because the function parses ELF and sets global variables.
	 * To the same to the binary loader for symmetry.
	 */
	if (iself)
		ret = copy_lm32_elf(buf, noload, size);
	else
		ret = copy_lm32(buf, noload, size, 0);
	if (!noload)
		rst_lm32(0);
	free(buf);
	return ret;
}

/* Set, or read, a variable. We already loaded to memory the file */
static int varaction_lm32(char *fname, char *action)
{
	char vname[64], sname[64];
	char stmp[256];
	int i, write, vvalue, saddr;
	FILE *f;
	char eq;

	if (!global_strptr) {
		fprintf(stderr, "%s: Can't execute \"%s\" on a non-elf file\n",
			prgname, action);
		return -1;
	}
	i = sscanf(action, "%[^=]%c%i", vname, &eq, &vvalue);
	if (i < 2 || eq != '=') {
		fprintf(stderr, "%s: Can't parse action \"%s\"\n",
			prgname, action);
		return -1;
	}
	if (i == 3)
		write = 1;
	else
		write = 0;

	/* Open "nm" (lazy me)" to find the variable's address */
	sprintf(stmp, "nm %s", fname);
	f = popen(stmp, "r");
	if (!f) {
		fprintf(stderr, "%s: Can't run \"%s\" (%s)\n",
			prgname, stmp, strerror(errno));
		return -1;
	}
	while (fgets(stmp, sizeof(stmp), f)) {
		if (sscanf(stmp, "%x %*c %s", &saddr, sname) != 2)
			continue;
		if (!strcmp(vname, sname))
			break;
	}
	if (feof(f)) {
		fprintf(stderr, "%s: no symbol \"%s\" int \"%s\"\n",
			prgname, sname, fname);
		pclose(f);
		return -1;
	}
	pclose(f);

	/* NOTE: we must not convert endianness here: it's bitwise ok */
	if (write) {
		/* FIXME: check it is in a writable section */
		fpga_writel(vvalue, saddr);
	} else {
		vvalue = fpga_readl(saddr);
		printf("%s = %i (0x%08x)\n", vname, vvalue, vvalue);
	}
	return 0;
}

int main(int argc, char **argv)
{
	int ret;
	int i, noload = 0;

	prgname = argv[0];
	if (argc > 1 && !strcmp(argv[1], "-n")) {
		noload = 1;
		argv++;
		argc--;
	}

	if (argc < 2) {
		fprintf(stderr, "%s: Use: \"%s [-n] <filename> "
			"[<var>=<value> ...]\"\n", prgname, prgname);
	}
	ret = load_lm32(argv[1], noload);
	if (ret)
		exit(1);

	for (i = 2; i < argc; i++)
		if (varaction_lm32(argv[1], argv[i]))
			exit(1);
	return 0;
}
