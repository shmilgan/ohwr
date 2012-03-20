/**
 * @file ad9516o.h
 *
 * @brief AD9516-O clock generator definitions
 *
 * The AD9516-O is controller through an SPI interface. This file
 * provides definitions for the different addresses of the module
 * to make its configuration easier to read.
 *
 * Copyright (c) 2009 CERN
 * @author Emilio G. Cota <emilio.garcia.cota@cern.ch>
 *
 * @section license_sec License
 * Released under the GPL v2. (and only v2, not any later version)
 */
#ifndef _AD9516O_H_
#define _AD9516O_H_

/*
 * AD9516-O Adress Map
 */
#define AD9516_SERIALPORT	0x00
#define AD9516_READBACK	0x04
#define AD9516_PDF_CP		0x10
#define AD9516_RCOUNT_LSB	0x11
#define AD9516_RCOUNT_MSB	0x12
#define AD9516_ACOUNT		0x13
#define AD9516_BCOUNT_LSB	0x14
#define AD9516_BCOUNT_MSB	0x15
#define AD9516_PLL1		0x16
#define AD9516_PLL2		0x17
#define AD9516_PLL3		0x18
#define AD9516_PLL4		0x19
#define AD9516_PLL5		0x1A
#define AD9516_PLL6		0x1B
#define AD9516_PLL7		0x1C
#define AD9516_PLL8		0x1D
#define AD9516_PLL9		0x1E
#define AD9516_PLLREADBACK	0x1F

/* Fine Delay Adjust: OUT6 to OUT9 */
#define AD9516_OUT6DELAY_BP	0xA0
#define AD9516_OUT6DELAY_FS	0xA1
#define AD9516_OUT6DELAY_FR	0xA2
#define AD9516_OUT7DELAY_BP	0xA3
#define AD9516_OUT7DELAY_FS	0xA4
#define AD9516_OUT7DELAY_FR	0xA5
#define AD9516_OUT8DELAY_BP	0xA6
#define AD9516_OUT8DELAY_FS	0xA7
#define AD9516_OUT8DELAY_FR	0xA8
#define AD9516_OUT9DELAY_BP	0xA9
#define AD9516_OUT9DELAY_FS	0xAA
#define AD9516_OUT9DELAY_FR	0xAB

/* LVPECL Outputs */
#define AD9516_LVPECL_OUT0	0xF0
#define AD9516_LVPECL_OUT1	0xF1
#define AD9516_LVPECL_OUT2	0xF2
#define AD9516_LVPECL_OUT3	0xF3
#define AD9516_LVPECL_OUT4	0xF4
#define AD9516_LVPECL_OUT5	0xF5

/* LVDS/CMOS Outputs */
#define AD9516_LVCMOS_OUT6	0x140
#define AD9516_LVCMOS_OUT7	0x141
#define AD9516_LVCMOS_OUT8	0x142
#define AD9516_LVCMOS_OUT9	0x143

/* LVPECL Channel Dividers */
#define AD9516_PECLDIV0_1	0x190
#define AD9516_PECLDIV0_2	0x191
#define AD9516_PECLDIV0_3	0x192
#define AD9516_PECLDIV1_1	0x193
#define AD9516_PECLDIV1_2	0x194
#define AD9516_PECLDIV1_3	0x195
#define AD9516_PECLDIV2_1	0x196
#define AD9516_PECLDIV2_2	0x197
#define AD9516_PECLDIV2_3	0x198

/* LVDS/CMOS Channel Dividers */
#define AD9516_CMOSDIV3_1	0x199
#define AD9516_CMOSDIV3_PHO	0x19A
#define AD9516_CMOSDIV3_2	0x19B
#define AD9516_CMOSDIV3_BYPASS	0x19C
#define AD9516_CMOSDIV3_DCCOFF	0x19D
#define AD9516_CMOSDIV4_1	0x19E
#define AD9516_CMOSDIV4_PHO	0x19F
#define AD9516_CMOSDIV4_2	0x1A0
#define AD9516_CMOSDIV4_BYPASS	0x1A1
#define AD9516_CMOSDIV4_DCCOFF	0x1A2

/* VCO Divider and CLK Input */
#define AD9516_VCO_DIVIDER	0x1E0
#define AD9516_INPUT_CLKS	0x1E1

/* System */
#define AD9516_POWDOWN_SYNC	0x230

/* Update All Registers */
#define AD9516_UPDATE_ALL	0x232

int shw_ad9516_init();
int shw_use_external_reference(int enable);

#endif /* _AD9516O_H_ */