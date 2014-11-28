#ifndef __LIBWR_HWIU_H__
#define __LIBWR_HWIU_H__

#define HWIU_INFO_START 0
#define HWIU_INFO_WORDS_START 1
#define HWIU_RD_TIMEOUT  30
#define HWIU_INFO_WORDS  4
#define HWIU_STRUCT_VERSION 1

struct gw_info {
	uint8_t ver_minor, ver_major;
	uint8_t nwords;
	uint8_t struct_ver;
	uint8_t build_no, build_year, build_month, build_day;
	uint32_t switch_hdl_hash;
	uint32_t general_cores_hash;
	uint32_t wr_cores_hash;
} __attribute__ ((packed));

int shw_hwiu_gwver(struct gw_info *info);

#endif /* __LIBWR_HWIU_H__ */
