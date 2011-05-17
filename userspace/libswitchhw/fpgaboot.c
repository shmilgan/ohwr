#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include <dirent.h>
#include <string.h>

#include <sys/stat.h>

#include <hw/pio.h>
#include <hw/fpgaboot.h>
#include <hw/trace.h>
#include <hw/util.h>
#include <hw/fpga_regs.h>
#include <hw/clkb_io.h>

#include "minilzo.h"

// directory (on the switch filesystem) containing the FPGA bitstreams

#define DEFAULT_FPGA_IMAGE_DIR "/wr/firmware"

static char fpga_image_dir[128] = DEFAULT_FPGA_IMAGE_DIR;

// firmware names for both FPGAs. Can be changed by calling shw_request_fpga_firmware()

static int force_new_firmware = 0;
static char *firmware_main = "board_test";
static char *firmware_clkb = "dmtd_pll_test";

#define REVID_MAGIC_VALUE 0xdeadbeef

#define REVID_REG_MAGIC 0x0
#define REVID_REG_FW_HASH 0x8
#define REVID_REG_FW_REVISION 0x4


static int get_fpga_revid(int fpga_id, uint32_t *fw_hash, uint32_t *rev_id)
{
	if(fpga_id == FPGA_ID_MAIN)
	{
// check the magic register value
		if(_fpga_readl(FPGA_BASE_REVID + REVID_REG_MAGIC) != REVID_MAGIC_VALUE) return -1;

		*fw_hash = _fpga_readl(FPGA_BASE_REVID +  REVID_REG_FW_HASH);
		*rev_id = _fpga_readl(FPGA_BASE_REVID +  REVID_REG_FW_REVISION);
		return 0;
	} else if(fpga_id == FPGA_ID_CLKB) {
 		if(shw_clkb_read_reg(CLKB_BASE_REVID + REVID_REG_MAGIC) != REVID_MAGIC_VALUE) return -1;

		*fw_hash = shw_clkb_read_reg(CLKB_BASE_REVID +  REVID_REG_FW_HASH);
		*rev_id =  shw_clkb_read_reg(CLKB_BASE_REVID +  REVID_REG_FW_REVISION);
		return 0;
	};

	return -1;
}

static int read_le32(FILE *f, uint32_t *rval)
{
#if __BYTE_ORDER==__LITTLE_ENDIAN
 return fread(rval, 4, 1, f) == 4 ? -1 : 0;
#else
  #error "Big endian architectures not supported yet :("
#endif
}

static char * read_string(FILE *f)
{
	char buf[1024];
	uint32_t len;

	if(read_le32(f, &len) < 0)
	 return NULL;

	if(len > sizeof(buf)-1)
	 return NULL;

	fread(buf, 1, len, f);
	buf[len]=0;

	return strdup(buf);
}

static int read_image_header(FILE *f,  struct fpga_image_entry *ent)
{
	struct fpga_image_header hdr;
	int rc = 0;

	if( fread(&hdr, sizeof(struct fpga_image_header), 1, f) != 1)
	  return -1;

  if(memcmp(hdr.magic, FPGA_IMAGE_MAGIC, 4))
 		return -1;

	ent->fpga_name = read_string(f);
	ent->fw_name = read_string(f);
	rc = read_le32(f, &ent->hash_reg);
  rc |= read_le32(f, &ent->revision);
  rc |= read_le32(f, &ent->size);
	rc |= read_le32(f, &ent->compressed_size);

	if(!ent->fpga_name || !ent->fw_name || rc) return -1;

	return 0;
}

const char *expand_fpga_id(int fpga_id)
{
	if(fpga_id == FPGA_ID_MAIN)
		return "mch_main";
	else if(fpga_id == FPGA_ID_CLKB)
		return "mch_clkb";
  else return "unknown?";
}


static int stat_is_file(const char *name)
{
	struct stat sb;
	if(stat(name, &sb) != 0) return 0;
	return S_ISREG(sb.st_mode);
}



static int find_fpga_image(int fpga_id, const char *fw_name, int rev_id, uint32_t fw_hash, struct fpga_image_entry *ent_h)
{
	FILE *f;
	struct dirent **namelist;
	int n;

	char latest_image_name[1024];
	int found = 0;
	int max_rev = -1;

	n = scandir(fpga_image_dir, &namelist, 0, alphasort);

	if(n<0)
	  {
	    TRACE(TRACE_FATAL, "Scanning the FPGA image directory (%s) failed.", fpga_image_dir);
	    return -1;
	  }

	while(n--)
	{
		char namebuf[1024];

		strncpy(namebuf, fpga_image_dir, 1024);
		strncat(namebuf, "/", 1024);
		strncat(namebuf, namelist[n]->d_name, 1024);

		if(!stat_is_file(namebuf)) continue;

		f = fopen(namebuf, "rb");
		if(f)
		{
			struct fpga_image_entry ent;
			if(!read_image_header(f, &ent))
			{
			  TRACE(TRACE_INFO,"CheckFW: %s [%s] rev %d", ent.fpga_name, ent.fw_name, ent.revision);
			  if(rev_id) // name-based lookup
				{
					if(!strcasecmp(ent.fpga_name, expand_fpga_id(fpga_id))
					   && !strcasecmp(ent.fw_name, fw_name))
					   {
					  	 found = 1;

					  	 if((int)ent.revision > max_rev)
					  	 {
					  		 max_rev = ent.revision;
					  		 strncpy(latest_image_name, namebuf, 1024);
					  	 }
					   }
				} else { // revid-based lookup: we look for newer firmware with matching revid

					if(ent.hash_reg == fw_hash && (int)ent.revision > max_rev)
					{
					  found = 1;
					  max_rev = ent.revision;
					  strncpy(latest_image_name, namebuf, 1024);
					}
				}

			}
			shw_free(namelist[n]);
			fclose(f);
		}
	}
	shw_free(namelist);


	if(!found)
		return -1;

  if(rev_id >= 0 && max_rev <= rev_id)
 	  return -1;

	f = fopen(latest_image_name, "rb");
	read_image_header(f, ent_h);

	TRACE(TRACE_INFO, "Found most recent image: %s (FPGA: %s, firmware: %s, revision: %d)", latest_image_name, ent_h->fpga_name, ent_h->fw_name, ent_h->revision);

	ent_h -> image_buf = shw_malloc(ent_h->compressed_size);
	fread(ent_h->image_buf, 1, ent_h->compressed_size, f);
	fclose(f);

  return 0;
}

static int uncompress_and_boot_fpga(int fpga_id, struct fpga_image_entry  *ent)
{
    uint8_t *bitstream;
    uint32_t dec_size, img_offset = 0;


    bitstream = shw_malloc(ent->size);
    dec_size = ent->size;

    int r = lzo1x_decompress(ent->image_buf, ent->compressed_size, bitstream, (lzo_uint*) &dec_size, NULL);

            if (r != LZO_E_OK || dec_size != ent->size)
            {
                    TRACE(TRACE_ERROR, "Image decompression failed.");
                    shw_free(ent->image_buf);
                    shw_free(bitstream);
                    return -1;
            }

	int rc = shw_load_fpga_bitstream(fpga_id, bitstream, ent->size);


	shw_free(bitstream);
  shw_free(ent->image_buf);

  return rc;

}




int shw_boot_fpga(int fpga_id)
{
  char *fw_name;
  const char *fpga_name;
  int rev_id;
  uint32_t fw_hash;
  struct fpga_image_entry ent;
  int rc;

  fpga_name = expand_fpga_id(fpga_id);

  TRACE(TRACE_INFO, "%s: reading REV_ID...", fpga_name);

  if(get_fpga_revid (fpga_id, &fw_hash, (uint32_t *)&rev_id) < 0)
    rev_id = -1;

  switch(fpga_id)
    {
    case FPGA_ID_MAIN:
      fw_name = firmware_main; break;
    case FPGA_ID_CLKB:
      fw_name = firmware_clkb; break;
    }

  if(rev_id < 0 || force_new_firmware)
    {
      TRACE(TRACE_INFO, "%s: invalid REV_ID or forced firmware update. Trying to boot the FPGA with the firmware: %s", fpga_name, fw_name)
	rc = find_fpga_image(fpga_id, fw_name, -1, 0, &ent);
      if(rc < 0) {
	TRACE(TRACE_FATAL, "%s: unable to find any image for FPGA!", fpga_name);
	shw_exit_fatal();
	return -1;
      }

      return uncompress_and_boot_fpga(fpga_id, &ent);
    } else {
    TRACE(TRACE_INFO, "%s: got REV_ID (rev = %d, hash = %x).  Looking for newer firmware", fpga_name, rev_id, fw_hash);
    rc = find_fpga_image(fpga_id, fw_name, rev_id, fw_hash, &ent);

    if(rc < 0) {

      TRACE(TRACE_INFO, "%s: no more recent image found", fpga_name);
      return 0;
    } else return uncompress_and_boot_fpga(fpga_id, &ent);
  }

  return -1;
}

int shw_request_fpga_firmware(int fpga_id, const char *firmware_name)
{
  switch(fpga_id)
    {
    case FPGA_ID_MAIN:
      firmware_main = strdup(firmware_name); break;
    case FPGA_ID_CLKB:
      firmware_clkb = strdup(firmware_name); break;
    }
  return 0;
}

int shw_set_fpga_firmware_path(const char *path)
{
	strncpy(fpga_image_dir, path, sizeof(fpga_image_dir));
	TRACE(TRACE_INFO,"FPGA firmware directory set to %s", fpga_image_dir);
	return 0;
}

void shw_fpga_force_firmware_reload()
{
    force_new_firmware = 1;
}

int shw_fpgaboot_init()
{
	mblaster_init();
	//strncpy(fpga_image_dir, DEFAULT_FPGA_IMAGE_DIR, sizeof(fpga_image_dir));
	return 0;
}
