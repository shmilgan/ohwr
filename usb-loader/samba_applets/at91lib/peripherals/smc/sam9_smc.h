/*
 * linux/arch/arm/mach-at91/sam9_smc.
 *
 * Copyright (C) 2008 Andrew Victor
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

struct  sam9_smc_config {
	/* Setup register */
	unsigned char ncs_read_setup;
	unsigned char nrd_setup;
	unsigned char ncs_write_setup;
	unsigned char nwe_setup;

	/* Pulse register */
	unsigned char ncs_read_pulse;
	unsigned char nrd_pulse;
	unsigned char ncs_write_pulse;
	unsigned char nwe_pulse;

	/* Cycle register */
	unsigned short read_cycle;
	unsigned short write_cycle;

	/* Mode register */
	unsigned int mode;
	unsigned char tdf_cycles:4;
} ;

extern void  sam9_smc_configure(int cs, struct sam9_smc_config* config);
