/* Altera's MicroBlaster code for programming Cyclone3 FPGAs in Passive Seral mode */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include <dirent.h>

#include <hw/pio.h>
#include <hw/fpgaboot.h>
#include <hw/trace.h>
#include <hw/util.h>

#define SIG_DCLK			0
#define	SIG_NCONFIG			1
#define SIG_DATA0			2
#define SIG_NSTATUS			3
#define SIG_CONFDONE		4

#define INIT_CYCLE			50
#define RECONF_COUNT_MAX	5
#define	CHECK_EVERY_X_BYTE  10240

#define CLOCK_X_CYCLE		0

static inline void Dump2Port(const pio_pin_t **pinmap, int signal, int data, int clk );
static inline void ProgramByte(const pio_pin_t **pinmap, int one_byte );

static const pio_pin_t *io_mapping_main[] = {
        PIN_main_fpga_dclk,
        PIN_main_fpga_nconfig,
        PIN_main_fpga_data,
        PIN_main_fpga_nstatus,
        PIN_main_fpga_confdone
};

static const pio_pin_t *io_mapping_clkb[] = {
        PIN_clkb_fpga_dclk,
        PIN_clkb_fpga_nconfig,
        PIN_clkb_fpga_data,
        PIN_clkb_fpga_nstatus,
        PIN_clkb_fpga_confdone
};

int mblaster_init()
{
    int i;
    TRACE(TRACE_INFO, "Configuring the FPGA setup signals");

    for(i=0;i<5;i++)
    {
            shw_pio_configure(io_mapping_main[i]);
            shw_pio_configure(io_mapping_clkb[i]);
    }
}

static int mblaster_boot_fpga(const  uint8_t *bitstream, uint32_t bitstream_size, const pio_pin_t **pinmap)
{
	int			program_done = 0;			/* programming process (configuration and initialization) done = 1 */
	int			seek_position = 0;			/* file pointer position */
	int			configuration_count = RECONF_COUNT_MAX;	/* # reprogramming after error */
	int			one_byte = 0;				/* the byte to program */
	long int	i = 0;						/* standard counter variable */
	int			confdone_ok = 1;			/* CONF_DONE pin. '1' - error */
	int			nstatus_ok = 0;				/* NSTATUS pin. '0' - error */
	int			clock_x_cycle = CLOCK_X_CYCLE; /* Clock another 'x' cycles during user mode ( not necessary, for debug purpose only) */
	int			BBMV=0;
	int			BBII=0;




	/* Start configuration */
	while ( !program_done && (configuration_count>0) )
	{

		/* Drive a transition of 0 to 1 to NCONFIG to indicate start of configuration */
		Dump2Port(pinmap,  SIG_NCONFIG, 0, 0 );
		shw_udelay(1);
		Dump2Port(pinmap,  SIG_NCONFIG, 1, 0 );
        shw_udelay(1);

		/* Loop through every single byte */
		for ( i = 0; i < bitstream_size; i++ )
		{
			/*one_byte = fgetc( (FILE*) finputid );*/
			one_byte = bitstream[i];

			/* Progaram a byte */
			ProgramByte(pinmap,  one_byte );

			/* Check for error through NSTATUS for every 10KB programmed and the last byte */
			if ( !(i % CHECK_EVERY_X_BYTE) || (i == bitstream_size - 1) )
			{
                shw_udelay(100);
                //printf("I = %d\n", i);
				nstatus_ok = shw_pio_get(pinmap[SIG_NSTATUS]); //CheckSignal( SIG_NSTATUS );

				if ( !nstatus_ok )
				{
                    printf("status dupa!\n");


					program_done = 0;
					break;
				}
				else
					program_done = 1;
			}
		}

		configuration_count--;

		if ( !program_done )
			continue;

		/* Configuration end */
		/* Check CONF_DONE that indicates end of configuration */
		confdone_ok = !shw_pio_get(pinmap[SIG_CONFDONE]); //!CheckSignal( SIG_CONFDONE );

		if ( confdone_ok )
		{
			TRACE(TRACE_ERROR ,"Configuration done but contains error... CONF_DONE is %s", (confdone_ok? "LOW":"HIGH") );
			program_done = 0;

			if ( configuration_count == 0 )
				break;
		}

		/* if contain error during configuration, restart configuration */
		if ( !program_done )
			continue;

		/* program_done = 1; */

		/* Start initialization */
		/* Clock another extra DCLK cycles while initialization is in progress
		   through internal oscillator or driving clock cycles into CLKUSR pin */
		/* These extra DCLK cycles do not initialize the device into USER MODE */
		/* It is not required to drive extra DCLK cycles at the end of
		   configuration													   */
		/* The purpose of driving extra DCLK cycles here is to insert some delay
		   while waiting for the initialization of the device to complete before
		   checking the CONFDONE and NSTATUS signals at the end of whole
		   configuration cycle 											       */
		for ( i = 0; i < INIT_CYCLE; i++ )
		{
			Dump2Port( pinmap, SIG_DCLK, 0, 0 );
			Dump2Port( pinmap, SIG_DCLK, 1, 0 );
		}
		/* Initialization end */

		nstatus_ok = shw_pio_get(pinmap[SIG_NSTATUS]); //CheckSignal( SIG_NSTATUS );
		confdone_ok = !shw_pio_get(pinmap[SIG_CONFDONE]); //!CheckSignal( SIG_CONFDONE );

		if ( !nstatus_ok || confdone_ok )
		{
			TRACE(TRACE_ERROR, "Initialization finished but contains error: NSTATUS is %s and CONF_DONE is %s. Exiting...", (nstatus_ok?"HIGH":"LOW"), (confdone_ok?"LOW":"HIGH") );
			program_done = 0;
			configuration_count = 0; /* No reconfiguration */
		}
	}

	/* Add another 'x' clock cycles while the device is in user mode.
	   This is not necessary and optional. Only used for debugging purposes */
	if ( clock_x_cycle > 0 )
	{
		TRACE(TRACE_INFO, "Clock another %d cycles in while device is in user mode...", clock_x_cycle );
		for ( i = 0; i < CLOCK_X_CYCLE; i++ )
		{
			Dump2Port( pinmap, SIG_DCLK, 0, 0 );
			Dump2Port( pinmap, SIG_DCLK, 1, 0 );
		}
	}

	if ( !program_done )
	{
		TRACE(TRACE_ERROR, "Error: Configuration not successful! Error encountered..." );
		return -1;
	}

	Dump2Port( pinmap, SIG_DCLK, 1, 0 );
	Dump2Port( pinmap, SIG_DATA0, 1, 0 );

	TRACE(TRACE_INFO, "Configuration successful!" );
    return 0;
}
/********************************************************************************/
/*	Name:			Dump2Port													*/
/*																				*/
/*	Parameters:		int signal, int data, int clk	 							*/
/*					- name of the signal (SIG_*).								*/
/*					- value to be dumped to the signal.							*/
/*					- assert a LOW to HIGH transition to SIG_DCLK togther with	*/
/*					  [signal].													*/
/*																				*/
/*	Return Value:	None.														*/
/*																				*/
/*	Descriptions:	Dump [data] to [signal]. If [clk] is '1', a clock pulse is	*/
/*					generated after the [data] is dumped to [signal].			*/
/*																				*/
/********************************************************************************/
static inline void mdelay()
{
        int i;
        for(i=0;i<10;i++) asm volatile("nop");
}
static inline void Dump2Port(const pio_pin_t **pinmap, int signal, int data, int clk )
{
//	fprintf(stderr,"d2p: %d %d %d %d\n", signal, data, clk);
    if(clk)
        shw_pio_set0(pinmap[SIG_DCLK]);

    shw_pio_set(pinmap[signal], data);
   // mdelay();

	if(clk)
        shw_pio_set1(pinmap[SIG_DCLK]);
    //mdelay();

}

static inline void ProgramByte(const pio_pin_t **pinmap, int one_byte )
{
	int	bit = 0;
	int i = 0;

	/* write from LSb to MSb */
	for ( i = 0; i < 8; i++ )
	{
		bit = one_byte >> i;
		bit = bit & 0x1;

		/* Dump to DATA0 and insert a positive edge pulse at the same time */
		Dump2Port( pinmap, SIG_DATA0, bit, 1 );
	}
}


int shw_load_fpga_bitstream(int fpga_id, uint8_t *bitstream, uint32_t bitstream_size)
{
    switch(fpga_id)
    {
        case FPGA_ID_MAIN:
            return mblaster_boot_fpga(bitstream, bitstream_size, io_mapping_main);
            break;
        case FPGA_ID_CLKB:
            return mblaster_boot_fpga(bitstream, bitstream_size, io_mapping_clkb);
            break;
    }

    return 0;
}

