/* The FPGA bootloader 

	 Short explanation of the booting process:
	 - each FPGA has a "revision ID" block at address 0x0. This block
	   contains a magic number (so the CPU can detect the presence of a
	   valid firmware in the FPGA), the revision of the firmware and a
	   hash of the name of the currently loaded bitstream.
	 - the loader first attempts to read the Revid. If there's no valid Revid,
	   it searches for the most recent firmware with matching name (and if no 
	   explicit firmware name is given, it just loads the most recent version of
	   the default firmware). 
	 - if a valid Revid was found, it checks if the requested firmware name
	 	 differs from the currently running firmware (by comparing the name hashes).
	 	 If so, it searches for the most recent version of the new firmware and loads it.
	 - if the name of the firmware running is the same as the name of the requested
	   firmware, it just looks for an update (and eventually loads it). 
	
 	 Note that this file only deals with the configuration files, versioning, etc. The actual 
 	 low-level bootloader is in mblaster.c
*/
	 
	 
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

/* directory (on the switch filesystem) containing the FPGA bitstreams */
#define DEFAULT_FPGA_IMAGE_DIR "/wr/firmware"

/* Definition of Revid registers/magic value */
#define REVID_MAGIC_VALUE 0xdeadbeef

#define REVID_REG_MAGIC 0x0
#define REVID_REG_FW_HASH 0x8
#define REVID_REG_FW_REVISION 0x4

static char fpga_image_dir[128] = DEFAULT_FPGA_IMAGE_DIR;

/* firmware names for both FPGAs. Can be changed by calling shw_request_fpga_firmware().
   When force_new_firmware != 0, the FW is reloaded without any Revid checks. */
static int force_new_firmware = 0;
static char *firmware_main = "board_test";
static char *firmware_clkb = "dmtd_pll_test";


/* Reads the Revid block from a given FPGA (Main/Timing). Returns 0 on success,
   negative when no valid Revid has been found. The hash and revision id are stored
   at (fw_hash) and (rev_id). */
static int get_fpga_revid(int fpga_id, uint32_t *fw_hash, uint32_t *rev_id)
{
	if(fpga_id == FPGA_ID_MAIN)
	{
		/* Check the magic register value. */
		if(_fpga_readl(FPGA_BASE_REVID + REVID_REG_MAGIC) != REVID_MAGIC_VALUE) return -1;

		*fw_hash = _fpga_readl(FPGA_BASE_REVID +  REVID_REG_FW_HASH);
		*rev_id = _fpga_readl(FPGA_BASE_REVID +  REVID_REG_FW_REVISION);
		return 0;
	} else if(fpga_id == FPGA_ID_CLKB) {
		/* For the Timing FPGA, must use SPI-based access method */
 		if(shw_clkb_read_reg(CLKB_BASE_REVID + REVID_REG_MAGIC) != REVID_MAGIC_VALUE) return -1;

		*fw_hash = shw_clkb_read_reg(CLKB_BASE_REVID +  REVID_REG_FW_HASH);
		*rev_id =  shw_clkb_read_reg(CLKB_BASE_REVID +  REVID_REG_FW_REVISION);
		return 0;
	};

	return -1;
}

/* Reads a 32-bit little endian word from a file (rval). Returns negative on failure, 0 on success. */
static int read_le32(FILE *f, uint32_t *rval)
{
#if __BYTE_ORDER==__LITTLE_ENDIAN
 return fread(rval, 4, 1, f) == 4 ? -1 : 0;
#else
  #error "Big endian architectures not supported yet :("
#endif
}

/* Reads a string from file, consisting of a 32-bit word containing its length followed by the
   actual string. Uses strdup(), so the result must be freed after use. */

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

/* Reads the header of the FPGA bitstream file. Returns 0 on success. */
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

/* Checks if the filesystem entry "name" is an ordinary file. */
static int stat_is_file(const char *name)
{
	struct stat sb;
	if(stat(name, &sb) != 0) return 0;
	return S_ISREG(sb.st_mode);
}


/* Searches for the most recent FPGA image for the FPGA (fpga_id) and firmware name (fw_name). (rev_id) and (fw_hash)
   are the values read from the Revid block (when rev_id < 0, no Revid block was found). Returns the allocated FPGA
   bitstream entry (ent_h) containing the contents of the config file with the most recent FW version and 0 if a newer
   firmware has been found. If there's no newer firmware, returns a negative value. */
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
			  /* No firmware in the FPGA? Find one with matching fw_name and the most recent revision ID */
			  if(rev_id) 
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
		  	/* Firmware already present? Look for an update (matching hash, rev > current revision) */
				} else { 

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

	/* Read the contents of the compressed bitstream file. */
	ent_h -> image_buf = shw_malloc(ent_h->compressed_size);
	fread(ent_h->image_buf, 1, ent_h->compressed_size, f);
	fclose(f);

  return 0;
}

/* Uncompresses the bitstream from (ent) and executes the low-level loader for (fpga_id). */
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

/* Boots the FPGA (fpga_id). Returns 0 on success, negative on failure. */ 
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

	/* Try to read Revid block */
  if(get_fpga_revid (fpga_id, &fw_hash, (uint32_t *)&rev_id) < 0)
    rev_id = -1;

  switch(fpga_id)
    {
    case FPGA_ID_MAIN:
      fw_name = firmware_main; break;
    case FPGA_ID_CLKB:
      fw_name = firmware_clkb; break;
    }

	/* No Revid found or unconditional reboot */
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
    } else { /* Look for an update */
    	TRACE(TRACE_INFO, "%s: got REV_ID (rev = %d, hash = %x).  Looking for newer firmware", fpga_name, rev_id, fw_hash);
    	rc = find_fpga_image(fpga_id, fw_name, rev_id, fw_hash, &ent);

    	if(rc < 0) {
      	TRACE(TRACE_INFO, "%s: no more recent image found", fpga_name);
      	return 0;
    	} else return uncompress_and_boot_fpga(fpga_id, &ent);
  }

  return -1;
}

/* Requests the firmware for a given FPGA to be loaded during libswitchhw initialization.
	 Executed by the HAL prior to initializing the libswitchhw. */
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

/* Sets the path to the directory with FPGA images */
int shw_set_fpga_firmware_path(const char *path)
{
	strncpy(fpga_image_dir, path, sizeof(fpga_image_dir));
	TRACE(TRACE_INFO,"FPGA firmware directory set to %s", fpga_image_dir);
	return 0;
}

/* Forces an unconditional FPGA reload. */
void shw_fpga_force_firmware_reload()
{
    force_new_firmware = 1;
}

/* Initializes the FPGA bootloader (currently just calls the low-level init function) */
int shw_fpgaboot_init()
{
	mblaster_init();
	return 0;
}
