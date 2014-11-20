#include <stdio.h>
#include <inttypes.h>
#include <stddef.h>
#include <fpga_io.h>
#include <regs/hwiu-regs.h>
#include <libwr/hwiu.h>

#define hwiu_write(reg, val) \
  _fpga_writel(FPGA_BASE_HWIU + offsetof(struct HWIU_WB, reg), val)

#define hwiu_read(reg) \
  _fpga_readl(FPGA_BASE_HWIU + offsetof(struct HWIU_WB, reg))

static int hwiu_read_word(uint32_t adr, uint32_t *data)
{
  uint32_t temp;
  int timeout = 0;

  temp = HWIU_CR_RD_EN | HWIU_CR_ADR_W(adr);
  hwiu_write(CR, temp);
  do {
    temp = hwiu_read(CR);
    ++timeout;
  } while( temp & HWIU_CR_RD_EN && timeout < HWIU_RD_TIMEOUT );

  if( timeout == HWIU_RD_TIMEOUT || temp & HWIU_CR_RD_ERR )
    return -1;

  *data = hwiu_read(REG_VAL);
  return 0;
}

int shw_hwiu_gwver(struct gw_info *info)
{
  uint32_t data[HWIU_INFO_WORDS+1];
  struct gw_info *s_data;
  int i;

  //read first word of info struct
  if( hwiu_read_word(HWIU_INFO_START, data) < 0 )
    return -1;

  s_data = (struct gw_info *)data;
  *info = *s_data;
  if( info->nwords != HWIU_INFO_WORDS ) {
    printf("nwords: sw=%u, hw=%u, ver=%u, data=%x\n", info->nwords, HWIU_INFO_WORDS, info->struct_ver, data[0]);
    return -1;
  }

  //now read info words
  for(i=0; i<info->nwords; ++i) {
    if( hwiu_read_word(HWIU_INFO_WORDS_START+i, data+i+1) < 0 )
      return -1;
  }

  *info = *( (struct gw_info*) data);

  return 0;
}
