#include "mb_io.h"
#include <stdio.h>

#include "cpu_io.h"

#define SIG_DCLK			0
#define	SIG_NCONFIG			1
#define SIG_DATA0			2
#define SIG_NSTATUS			3
#define SIG_CONFDONE		4

#define S_CUR				1
#define S_END				2
#define S_SET				0

#define INIT_CYCLE			50		
#define RECONF_COUNT_MAX	5      	
#define	CHECK_EVERY_X_BYTE  10240	

#define CLOCK_X_CYCLE		0	


static inline int CheckSignal			( int signal );
static inline void Dump2Port			( int signal, int data, int clk );
void	PrintError ( int configuration_count );
void	ProcessFileInput	( int finputid );
static inline void	ProgramByte			( int one_byte );
void	SetPortMode			( int mode );

const char VERSION[4] = "1.1";

struct pgm_io_mapping {
  int port;
  int pin;
  int mode;
  int dir;
  int invert;
};

static const struct pgm_io_mapping io_mapping_main [] = {
  { PIOB, 1, PIN_MODE_GPIO, 1, 0}, // SIG_DCLK
  { PIOB, 22, PIN_MODE_GPIO, 1, 0}, // SIG_NCONFIG
  { PIOB, 2, PIN_MODE_GPIO, 1, 0}, // SIG_DATA0
  { PIOB, 31, PIN_MODE_GPIO, 0, 0}, // SIG_NSTATUS
{ PIOB, 18, PIN_MODE_GPIO, 0, 1} // SIG_CONFDONE
};


static const struct pgm_io_mapping io_mapping_clkb [] = {
  { PIOB, 7, PIN_MODE_GPIO, 1, 0}, // SIG_DCLK
  { PIOB, 23, PIN_MODE_GPIO, 1, 0}, // SIG_NCONFIG
  { PIOB, 8, PIN_MODE_GPIO, 1, 0}, // SIG_DATA0
  { PIOB, 28, PIN_MODE_GPIO, 0, 0}, // SIG_NSTATUS
{ PIOB, 24, PIN_MODE_GPIO, 0, 1} // SIG_CONFDONE
};

struct {
  volatile uint8_t *port_in;
  volatile uint8_t *port_1;
  volatile uint8_t *port_0;
  uint32_t mask;
  uint32_t xorval;
} fast_io_map[5];

void port_init(struct pgm_io_mapping *io_mapping)
{
  int i;
  
  

  for(i=0; i<5; i++)
  {
	pio_set_state(io_mapping[i].port, io_mapping[i].pin, 1);
	pio_set_mode(io_mapping[i].port, io_mapping[i].pin, io_mapping[i].mode, io_mapping[i].dir);
  
	fast_io_map[i].port_in = pio_get_port_addr(io_mapping[i].port) + PIO_PDSR;
	fast_io_map[i].port_0 = pio_get_port_addr(io_mapping[i].port) + PIO_CODR;
	fast_io_map[i].port_1 = pio_get_port_addr(io_mapping[i].port) + PIO_SODR;

	fast_io_map[i].mask = (1 << io_mapping[i].pin);
	fast_io_map[i].xorval = io_mapping[i].invert ? (1 << io_mapping[i].pin) : 0;
  }
}

int mblaster_boot_fpga(const char *filename, int fpga_sel)
{
	int file_id;

	if(fpga_sel == FPGA_MAIN)
	    port_init(io_mapping_main);
	else if(fpga_sel == FPGA_CLKB)
	    port_init(io_mapping_clkb);
	else {
		fprintf( stderr, "Error: please select either MAIN or CLKB FPGA.", filename );
		return -1;
	}


	file_id = fopen_rbf( filename, "rb" );

	if ( file_id )
		fprintf( stdout, "Info: Programming file: \"%s\" opened...\n", filename );
	else
	{
		fprintf( stderr, "Error: Could not open programming file: \"%s\"\n", filename );
		return -1;
	}

	ProcessFileInput( file_id );


	if ( file_id )
		fclose_rbf(file_id);

  return 0;
}

/********************************************************************************/
/*	Name:			ProcessFileInput											*/
/*																				*/
/*	Parameters:		FILE* finputid												*/
/*					- programming file pointer.									*/
/*																				*/
/*	Return Value:	None.														*/
/*																				*/
/*	Descriptions:	Get programming file size, parse through every single byte	*/
/*					and dump to parallel port.									*/
/*																				*/
/*					Configuration Hardware is verified before configuration		*/
/*					starts.														*/
/*																				*/
/*					For every [CHECK_EVERY_X_BYTE] bytes, NSTATUS pin is		*/
/*					checked for error. When the file size reached, CONF_DONE	*/
/*					pin is checked for configuration status. Then, another		*/
/*					[INIT_CYCLE] clock cycles are dumped while initialization	*/
/*					is in progress.												*/
/*																				*/
/*					Configuration process is restarted whenever error found.	*/
/*					The maximum number of auto-reconfiguration is				*/
/*					[RECONF_COUNT_MAX].											*/
/*																				*/
/********************************************************************************/
void ProcessFileInput( int finputid )
{
	int			program_done = 0;			/* programming process (configuration and initialization) done = 1 */
	int			seek_position = 0;			/* file pointer position */
	long int	file_size = 0;				/* programming file size */
	int			configuration_count = RECONF_COUNT_MAX;	/* # reprogramming after error */
	int			one_byte = 0;				/* the byte to program */
	long int	i = 0;						/* standard counter variable */
	int			confdone_ok = 1;			/* CONF_DONE pin. '1' - error */
	int			nstatus_ok = 0;				/* NSTATUS pin. '0' - error */
	int			clock_x_cycle = CLOCK_X_CYCLE; /* Clock another 'x' cycles during user mode ( not necessary, for debug purpose only) */
	int			BBMV=0;
	int			BBII=0;

	/* Get file size */
	seek_position = fseek_rbf( finputid, 0, S_END );

	if ( seek_position )
	{
		fprintf( stderr, "Error: End of file could not be located!" );
		return;
	}

	file_size	= ftell_rbf( finputid );
	fprintf( stdout, "Info: Programming file size: %ld\n", file_size );
	
	/* Start configuration */
	while ( !program_done && (configuration_count>0) )
	{
		/* Reset file pointer and parallel port registers */
		fseek_rbf( finputid, 0, S_SET );

		fprintf( stdout, "\n***** Start configuration process *****\nPlease wait...\n" );

		/* Drive a transition of 0 to 1 to NCONFIG to indicate start of configuration */
		Dump2Port( SIG_NCONFIG, 0, 0 );
		Dump2Port( SIG_NCONFIG, 1, 0 );

		/* Loop through every single byte */
		for ( i = 0; i < file_size; i++ )
		{
			/*one_byte = fgetc( (FILE*) finputid );*/
			one_byte = fgetc_rbf( finputid );

			/* Progaram a byte */
			ProgramByte( one_byte );

			/* Check for error through NSTATUS for every 10KB programmed and the last byte */
			if ( !(i % CHECK_EVERY_X_BYTE) || (i == file_size - 1) )
			{
				nstatus_ok = CheckSignal( SIG_NSTATUS );

				if ( !nstatus_ok )
				{
					PrintError( configuration_count-1 );
					
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
		confdone_ok = CheckSignal( SIG_CONFDONE );

		if ( confdone_ok )
		{
			fprintf( stderr, "Error: Configuration done but contains error... CONF_DONE is %s\n", (confdone_ok? "LOW":"HIGH") );
			program_done = 0;
			PrintError( configuration_count );			
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
			Dump2Port( SIG_DCLK, 0, 0 );
			Dump2Port( SIG_DCLK, 1, 0 );
		}
		/* Initialization end */

		nstatus_ok = CheckSignal( SIG_NSTATUS );
		confdone_ok = CheckSignal( SIG_CONFDONE );

		if ( !nstatus_ok || confdone_ok )
		{
			fprintf( stderr, "Error: Initialization finish but contains error: NSTATUS is %s and CONF_DONE is %s. Exiting...", (nstatus_ok?"HIGH":"LOW"), (confdone_ok?"LOW":"HIGH") );
			program_done = 0; 
			configuration_count = 0; /* No reconfiguration */
		}
	}

	/* Add another 'x' clock cycles while the device is in user mode.
	   This is not necessary and optional. Only used for debugging purposes */
	if ( clock_x_cycle > 0 )
	{
		fprintf( stdout, "Info: Clock another %d cycles in while device is in user mode...\n", clock_x_cycle );
		for ( i = 0; i < CLOCK_X_CYCLE; i++ )
		{
			Dump2Port( SIG_DCLK, 0, 0 );
			Dump2Port( SIG_DCLK, 1, 0 );
		}
	}

	if ( !program_done )
	{
		fprintf( stderr, "\nError: Configuration not successful! Error encountered...\n" );
		return;
	}

	Dump2Port( SIG_DCLK, 1, 0 );
	Dump2Port( SIG_DATA0, 1, 0 );

	fprintf( stdout, "\nInfo: Configuration successful!\n" );

}

/********************************************************************************/
/*	Name:			CheckSignal													*/
/*																				*/
/*	Parameters:		int signal						 							*/
/*					- name of the signal (SIG_*).								*/
/*																				*/
/*	Return Value:	Integer, the value of the signal. '0' is returned if the	*/
/*					value of the signal is LOW, if not, the signal is HIGH.		*/
/*																				*/
/*	Descriptions:	Return the value of the signal.								*/
/*																				*/
/********************************************************************************/



static int inline CheckSignal( int signal )
{
  return (fast_io_map[signal].xorval ^ (_readl(fast_io_map[signal].port_in) & fast_io_map[signal].mask)) ? 1: 0;
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
static inline void Dump2Port( int signal, int data, int clk )
{
//	fprintf(stderr,"d2p: %d %d %d %d\n", signal, data, clk);

	if(clk)
	  _writel(fast_io_map[SIG_DCLK].port_0, fast_io_map[SIG_DCLK].mask)
	  
	if(!data)
  	  _writel(fast_io_map[signal].port_0, fast_io_map[signal].mask)
  	else
  	  _writel(fast_io_map[signal].port_1, fast_io_map[signal].mask)


	if(clk)
	  _writel(fast_io_map[SIG_DCLK].port_1, fast_io_map[SIG_DCLK].mask)

}

static inline void ProgramByte( int one_byte )
{
	int	bit = 0;
	int i = 0;

	/* write from LSb to MSb */
	for ( i = 0; i < 8; i++ )
	{
		bit = one_byte >> i;
		bit = bit & 0x1;
		
		/* Dump to DATA0 and insert a positive edge pulse at the same time */
		Dump2Port( SIG_DATA0, bit, 1 );
	}
}

/********************************************************************************/
/*	Name:			PrintError 													*/
/*																				*/
/*	Parameters:		int configuration_count										*/
/*					- # auto-reconfiguration left  								*/
/*																				*/
/*	Return Value:	None.														*/
/*																				*/
/*	Descriptions:	Print error message to standard error.						*/
/*																				*/
/********************************************************************************/
void PrintError( int configuration_count )
{
	if ( configuration_count == 0 )
		fprintf( stderr, "Error: Error in configuration #%d... \nError: Maximum number of reconfiguration reached. Exiting...\n", (RECONF_COUNT_MAX-configuration_count) );
	else
	{
		fprintf( stderr, "Error: Error in configuration #%d... Restart configuration. Ready? <Press any key to continue>\n", (RECONF_COUNT_MAX-configuration_count) );
	}
}
