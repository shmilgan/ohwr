#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "minilzo.h"
#include "md5.h"

static const char IMAGE_MAGIC[4] = { 'w','r','f','i' };

struct fpga_image_header {
  char magic[4];
  uint32_t num_images;
};

struct fpga_image_entry {
  char *fpga_name; // name of the FPGA (for example: MAIN, CLKB)
  char *fw_name;   // name of the firmware (for example: board_test, rtu_test)
  uint32_t hash_reg; // MD5 hash of the firmware ID.
  uint32_t revision;
  uint32_t size;
  uint32_t compressed_size;
};

void die(const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  fprintf(stderr, "Error: ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n\n");
  va_end(ap);
  exit(-1);
}

static char lzo_workmem[LZO1X_1_MEM_COMPRESS];

uint32_t calc_hash_reg(char *fpga_name, char *firmware_name)
{
	char hash_buf[1024];
	uint8_t hash_val[16];
	int i;

	strncpy(hash_buf, fpga_name, 256);
	strcat(hash_buf,"::");
	strncat(hash_buf, firmware_name, 256);


	md5_checksum(hash_buf, strlen(hash_buf), hash_val);
	for(i=0;i<4;i++) hash_val[i] ^= (hash_val[4+i] ^ hash_val[8+i] ^ hash_val[12+i]);

	return (uint32_t) hash_val[0] |
				 ((uint32_t) hash_val[1] << 8) |
				 ((uint32_t) hash_val[2] << 16) |
				 ((uint32_t) hash_val[3] << 24);
}

char *prepare_image(char *filename, char *fpga_name, char *firmware_name, int revision, struct fpga_image_entry *ent)
{
  char *rbf_buf;
  uint32_t size, size_cmp;
  FILE *f = fopen(filename, "rb");

  if(!f) die("can't open %s", filename);
  fseek(f, 0, SEEK_END);
  size = ftell(f);
  rewind(f);

  rbf_buf = malloc(size);
  fread(rbf_buf, 1, size, f);
  fclose(f);

  char *p = malloc(size);

  lzo1x_1_compress(rbf_buf, size, p, (lzo_uint *)&size_cmp, lzo_workmem);

  free(rbf_buf);

  ent->fpga_name = fpga_name;
  ent->fw_name = firmware_name;
	ent->hash_reg = calc_hash_reg(fpga_name, firmware_name);
  ent->revision = revision;
  ent->size = size;
  ent->compressed_size = size_cmp;

  return p;
}

int write_le32(FILE *f, uint32_t x)
{
#if __BYTE_ORDER==__LITTLE_ENDIAN
 return fwrite(&x, 4, 1, f);
#else
 #error "Big endian architectures not supported yet :("
#endif
}

int write_string(FILE *f, char *str)
{
		unsigned int length = strlen(str);
		write_le32(f, length);
		fwrite(str, 1, length, f);
		return length + 4;
}

int write_entry_header(FILE *f, struct fpga_image_entry *ent)
{
  int n;

	n=write_string(f, ent->fpga_name);
	n+=write_string(f, ent->fw_name);
	n+=write_le32(f, ent->hash_reg);
	n+=write_le32(f, ent->revision);
	n+=write_le32(f, ent->size);
	n+=write_le32(f, ent->compressed_size);

	return n;
}



main(int argc, char *argv[])
{
  struct fpga_image_header hdr;
  struct fpga_image_entry ent;

  if(argc < 2)
    {
			printf("MCH FPGA image generator (c) TW, CERN BE-Co-HT 2010\n");
      printf("usage: %s OUTPUT.img INPUT.rbf REVISION FPGA_NAME FIRMWARE_NAME\n",argv[0]);
      printf("       %s -h FPGA_NAME FIRMWARE_NAME\n",argv[0]);
      printf("-h option returns the FW_HASH register value for given FPGA/firmware names\n\n");
      return 0;
    }

	if(!strcmp(argv[1], "-h"))
	{
		printf("0x%08x", calc_hash_reg(argv[2], argv[3]));
		return 0;
	} else if (argc < 5)
		die("Not enough arguments");


  FILE *fout = fopen(argv[1], "wb");

	if(!fout)
		die("Can't open output file: %s\n", argv[1]);


  memcpy(hdr.magic, IMAGE_MAGIC, 4);
  hdr.num_images = 1;


  fwrite(&hdr, sizeof(struct fpga_image_header), 1, fout);


	char *bitstream = prepare_image(argv[2], argv[4], argv[5], atoi(argv[3]), &ent);

	write_entry_header(fout, &ent);
	fwrite(bitstream, 1, ent.compressed_size, fout);



  printf("Total image size: %d bytes.\n", ftell(fout));
  fclose(fout);

  return 0;
}
