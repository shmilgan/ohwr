
/* Base configuration (global dividers, output config, reference-independent) */
const struct ad9516_reg ad9516_base_config[] = {
{0x0000, 0x99},
{0x0001, 0x00},
{0x0002, 0x10},
{0x0003, 0xC3},
{0x0004, 0x00},
{0x0010, 0x7C},
{0x0011, 0x05},
{0x0012, 0x00},
{0x0013, 0x0C},
{0x0014, 0x12},
{0x0015, 0x00},
{0x0016, 0x05},
{0x0017, 0x88},
{0x0018, 0x07},
{0x0019, 0x00},
{0x001A, 0x00},
{0x001B, 0x00},
{0x001C, 0x02},
{0x001D, 0x00},
{0x001E, 0x00},
{0x001F, 0x0E},
{0x00A0, 0x01},
{0x00A1, 0x00},
{0x00A2, 0x00},
{0x00A3, 0x01},
{0x00A4, 0x00},
{0x00A5, 0x00},
{0x00A6, 0x01},
{0x00A7, 0x00},
{0x00A8, 0x00},
{0x00A9, 0x01},
{0x00AA, 0x00},
{0x00AB, 0x00},
{0x00F0, 0x0A},
{0x00F1, 0x0A},
{0x00F2, 0x0A},
{0x00F3, 0x0A},
{0x00F4, 0x08},
{0x00F5, 0x08},
{0x0140, 0x43},
{0x0141, 0x42},
{0x0142, 0x43},
{0x0143, 0x42},
{0x0190, 0x00},
{0x0191, 0x80},
{0x0192, 0x00},
{0x0193, 0xBB},
{0x0194, 0x00},
{0x0195, 0x00},
{0x0196, 0x00},
{0x0197, 0x00},
{0x0198, 0x00},
{0x0199, 0x00},
{0x019A, 0x00},
{0x019B, 0x11},
{0x019C, 0x20},
{0x019D, 0x00},
{0x019E, 0x11},
{0x019F, 0x00},
{0x01A0, 0x11},
{0x01A1, 0x20},
{0x01A2, 0x00},
{0x01A3, 0x00},
{0x01E0, 0x04},
{0x01E1, 0x02},
{0x0230, 0x00},
{0x0231, 0x00},
};

/* Config for 25 MHz VCTCXO reference (RDiv = 5, use REF1) */
const struct ad9516_reg ad9516_ref_tcxo[] = {
{0x0011, 0x05},
{0x0012, 0x00}, /* RDiv = 5 */
{0x001C, 0x02}  /* Use REF1 */
};

