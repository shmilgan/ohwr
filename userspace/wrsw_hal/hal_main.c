/* Main HAL file */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <getopt.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <libwr/trace.h>
#include <libwr/switch_hw.h>
#include <libwr/shw_io.h>

#include "wrsw_hal.h"
#include "rt_ipc.h"

#define MAX_CLEANUP_CALLBACKS 16

#define assert_init(proc) { int ret; if((ret = proc) < 0) return ret; }

static int daemon_mode= 0;
static hal_cleanup_callback_t cleanup_cb[MAX_CLEANUP_CALLBACKS];

/* Adds a function to be called during the HAL shutdown. */
int hal_add_cleanup_callback(hal_cleanup_callback_t cb)
{
	int i;
	for(i=0;i<MAX_CLEANUP_CALLBACKS;i++)
		if(!cleanup_cb[i])
		{
		  cleanup_cb[i] = cb;
		  return 0;
		}

	return -1;
}

/* Calls all cleanup callbacks */
static void call_cleanup_cbs()
{
  int i;

	TRACE(TRACE_INFO, "Cleaning up...");
	for(i=0;i<MAX_CLEANUP_CALLBACKS;i++)
		if(cleanup_cb[i]) cleanup_cb[i]();
}


/* Determines which FPGA bitstreams shall be loaded */
int hal_setup_fpga_images()
{
	char fpga_dir[128];

/* query the path to the firmware directory in the config file */
  if( hal_config_get_string("global.hal_firmware_path", fpga_dir, sizeof(fpga_dir)) < 0)
 		return -1;

//  shw_set_fpga_firmware_path(fpga_dir);

/* check if the config demands a particular bitstream (otherwise libwr will load the default ones) */
/*  if( !hal_config_get_string("global.main_firmware", fw_name, sizeof(fw_name)))
 		shw_request_fpga_firmware(FPGA_ID_MAIN, fw_name);

  if( !hal_config_get_string("global.clkb_firmware", fw_name, sizeof(fw_name)))
 		shw_request_fpga_firmware(FPGA_ID_CLKB, fw_name);*/

  return 0;
}


/* loads (load = 1) or unloads (load = 0) a WR kernel module (name). */
static int load_unload_kmod(const char *name, int load)
{
	static char modules_path[128];
	static int modules_path_valid = 0;
	char cmd[256];

	if(!modules_path_valid)
	{
		if(hal_config_get_string("global.hal_modules_path", modules_path, sizeof(modules_path)) < 0)
		{
			TRACE(TRACE_ERROR, "Unable to locate kernel modules directory!");
			return -1;
		}
		modules_path_valid = 1;
	}


	TRACE(TRACE_INFO, "%s kernel module '%s'", load ? "Loading" : "Unloading", name);
	snprintf(cmd, sizeof(cmd), "%s %s/%s", load ? "/sbin/insmod" : "/sbin/rmmod", modules_path, name);

	system(cmd);

	return 0;
}


/* Unloads all WR kernel modules during the shutdown */
static void unload_kernel_modules()
{
	char module_name[80];
	int index = 0;

	for(;;)
	{
		if(!hal_config_iterate("global.modules", index++, module_name, sizeof(module_name)))
			break;

		load_unload_kmod(module_name, 0);
	}
}


/* Loads all WR kernel modules specified in the HAL config file */
int hal_load_kernel_modules()
{
	char module_name[80];
	int index = 0;

	TRACE(TRACE_INFO, "Loading kernel modules...");

	for(;;)
	{
		if(!hal_config_iterate("global.modules", index++, module_name, sizeof(module_name)))
			break;

		assert_init(load_unload_kmod(module_name, 1));
	}

	hal_add_cleanup_callback(unload_kernel_modules);

	return 0;

}

void sighandler(int sig)
{
	TRACE(TRACE_ERROR, "signal caught (%d)!", sig);

	//Set state led to orange
	shw_io_write(shw_io_led_state_o,1);
	shw_io_write(shw_io_led_state_g,0);

	call_cleanup_cbs();
	exit(0);
}

int hal_shutdown()
{
	call_cleanup_cbs();
	return 0;
}

void hal_deamonize();

/* Main initialization function */
int hal_init()
{
	char sfp_db_path[1024];
	
	//trace_log_stderr();

	TRACE(TRACE_INFO,"HAL initializing...");

	memset(cleanup_cb, 0, sizeof(cleanup_cb));

/* Set up trap for some signals - the main purpose is to prevent the hardware from working
   when the HAL is shut down - launching the HAL on already initialized HW will freeze the system. */
	signal(SIGSEGV, sighandler);
	signal(SIGINT, sighandler);
	signal(SIGTERM, sighandler);
	signal(SIGILL, sighandler);

/* parse the configuration file and choose the bitstreams to load to the FPGAs */
	assert_init(hal_parse_config());
//	assert_init(hal_setup_fpga_images());

  if(!hal_config_get_string("global.sfp_database_path", sfp_db_path, sizeof(sfp_db_path)))
  {
 		if(shw_sfp_read_db(sfp_db_path) < 0) {
			TRACE(TRACE_ERROR, "Can't read SFP database (%s)", sfp_db_path);
		} else {
			TRACE(TRACE_INFO, "Loaded SFP database (%s)", sfp_db_path);
		}
	}
	
/* Perform a low-level hardware init, load bitstreams, initialize non-kernel drivers */
	assert_init(shw_init());

	assert_init(hal_init_timing());

/* Initialize port FSMs - see hal_ports.c */
	assert_init(hal_init_ports());

/* Create a WRIPC server for HAL public API */
	assert_init(hal_init_wripc());

  //everything is fine up to here, we can blink green LED
	shw_io_write(shw_io_led_state_o,0);
	shw_io_write(shw_io_led_state_g,1);
  

	if(daemon_mode)
		hal_deamonize();

	return 0;
}

/* Main loop update - polls for WRIPC requests and rolls the port state machines */
void hal_update()
{
	hal_update_wripc();
	hal_update_ports();
	shw_update_fans();
//	usleep(1000);
}

/* Turns a nice and well-behaving HAL into an evil servant of satan. */
void hal_deamonize()
{
 pid_t pid, sid;

    /* already a daemon */
    if ( getppid() == 1 ) return;

    /* Fork off the parent process */
    pid = fork();
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }
    /* If we got a good PID, then we can exit the parent process. */
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    /* At this point we are executing as the child process */

    /* Change the file mode mask */
    umask(0);

    /* Create a new SID for the child process */
    sid = setsid();
    if (sid < 0) {
        exit(EXIT_FAILURE);
    }

    /* Change the current working directory.  This prevents the current
       directory from being locked; hence not being able to remove it. */
    if ((chdir("/")) < 0) {
        exit(EXIT_FAILURE);
    }

    /* Redirect standard files to /dev/null */
    freopen( "/dev/null", "r", stdin);
    freopen( "/dev/null", "w", stdout);
    freopen( "/dev/null", "w", stderr);

}

static void show_help()
{
printf("WR Switch Hardware Abstraction Layer daemon (wrsw_hal)\n\
Usage: wrsw_hal [options], where [options] can be:\n\
-f       : force FPGA firmware reload\n\
-d       : fork into background (daemon mode)\n\
-x [code]: execute arbitrary Lua [code] before loading configuration file\n\
-c [file]: specify your own config file\n\n");
}

void hal_parse_cmdline(int argc, char *argv[])
{
	int opt;

	while((opt=getopt(argc, argv, "dfhx:c:")) != -1)
	{
		switch(opt)
		{
			case 'd':
				daemon_mode = 1;
				break;
			case 'x':
				hal_config_extra_cmdline(optarg);
				break;

			case 'c':
				hal_config_set_config_file(optarg);
				break;

	//		case 'f':
//				shw_fpga_force_firmware_reload();
//				break;
			case 'h':
				show_help();
				exit(0);
				break;
			default:
				fprintf(stderr,"Unrecognized option. Call %s -h for help.\n", argv[0]);
				break;
		}
	}
}

int main(int argc, char *argv[])
{

	trace_log_file("/dev/kmsg");
/* Prevent from running HAL twice - this will likely freeze the system */
	if(hal_check_running())
	{
		fprintf(stderr, "Fatal: There is another WR HAL instance running. We can't work together.\n\n");
		return -1;
	}

	hal_parse_cmdline(argc, argv);

	hal_init();


	for(;;) hal_update();
	hal_shutdown();

	return 0;
}
