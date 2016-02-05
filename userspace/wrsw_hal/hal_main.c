/* Main HAL file */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <getopt.h>
#include <time.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <libwr/wrs-msg.h>
#include <libwr/switch_hw.h>
#include <libwr/shw_io.h>
#include <libwr/sfp_lib.h>
#include <libwr/config.h>
#include <libwr/hal_shmem.h>

#include "wrsw_hal.h"
#include <rt_ipc.h>

#define MAX_CLEANUP_CALLBACKS 16
#define PORT_FAN_MS_PERIOD 250

static int daemon_mode = 0;
static hal_cleanup_callback_t cleanup_cb[MAX_CLEANUP_CALLBACKS];
static char *logfilename;
static char *dotconfigname = "/wr/etc/dot-config";

struct hal_shmem_header *hal_shmem;

/* Adds a function to be called during the HAL shutdown. */
int hal_add_cleanup_callback(hal_cleanup_callback_t cb)
{
	int i;
	for (i = 0; i < MAX_CLEANUP_CALLBACKS; i++)
		if (!cleanup_cb[i]) {
			cleanup_cb[i] = cb;
			return 0;
		}

	return -1;
}

/* Calls all cleanup callbacks */
static void call_cleanup_cbs(void)
{
	int i;

	pr_info("Cleaning up...\n");
	for (i = 0; i < MAX_CLEANUP_CALLBACKS; i++)
		if (cleanup_cb[i])
			cleanup_cb[i] ();
}

static void sighandler(int sig)
{
	pr_error("signal caught (%d)!\n", sig);

	//Set state led to orange
	shw_io_write(shw_io_led_state_o, 1);
	shw_io_write(shw_io_led_state_g, 0);

	call_cleanup_cbs();
	exit(0);
}

static int hal_shutdown(void)
{
	call_cleanup_cbs();
	return 0;
}

static void hal_daemonize(void);

/* Main initialization function */
static int hal_init(void)
{
	//trace_log_stderr();

	int line;
	pr_info("initializing...\n");

	memset(cleanup_cb, 0, sizeof(cleanup_cb));

	pr_info("Using file %s as dot-config\n", dotconfigname);
	line = libwr_cfg_read_file(dotconfigname);

	if (line == -1)
		pr_error("Unable to read dot-config file %s or error in line"
			 "1\n", dotconfigname);
	else if (line)
		pr_error("Error in dot-config file %s, error in line %d\n",
			 dotconfigname, -line);

	shw_sfp_read_db();

	/* Set up trap for some signals - the main purpose is to
	   prevent the hardware from working when the HAL is shut down
	   - launching the HAL on already initialized HW will freeze
	   the system. */
	signal(SIGSEGV, sighandler);
	signal(SIGINT, sighandler);
	signal(SIGTERM, sighandler);
	signal(SIGILL, sighandler);

	/* Low-level hw init, init non-kernel drivers */
	assert_init(shw_init());

	assert_init(hal_init_timing(logfilename));

	/* Initialize port FSMs and IPC/RPC - see hal_ports.c */
	assert_init(hal_port_init_all(logfilename));

	//everything is fine up to here, we can blink green LED
	shw_io_write(shw_io_led_state_o, 0);
	shw_io_write(shw_io_led_state_g, 1);

	if (daemon_mode)
		hal_daemonize();

	return 0;
}

/* Turns a nice and well-behaving HAL into an evil servant of satan. */
static void hal_daemonize(void)
{
	pid_t pid, sid;

	/* already a daemon */
	if (getppid() == 1)
		return;

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

	/* Redirect stdin to /dev/null -- keep output/error: they are logged */
	freopen("/dev/null", "r", stdin);
}

static void show_help(void)
{
	printf("WR Switch Hardware Abstraction Layer daemon (wrsw_hal)\n\
Usage: wrsw_hal [options], where [options] can be:\n\
-d       : fork into background (daemon mode)\n");
}

static void hal_parse_cmdline(int argc, char *argv[])
{
	int opt;

	while ((opt = getopt(argc, argv, "dhqvl:f:")) != -1) {
		switch (opt) {
		case 'd':
			daemon_mode = 1;
			break;

		case 'h':
			show_help();
			exit(0);
			break;

		case 'l':
			logfilename = optarg;
			break;

		case 'q': break; /* done in wrs_msg_init() */
		case 'v': break; /* done in wrs_msg_init() */

		case 'f':
			/* custom dot-config file */
			dotconfigname = optarg;
			break;

		default:
			fprintf(stderr,
				"Unrecognized option. Call %s -h for help.\n",
				argv[0]);
			break;
		}
	}
}

int main(int argc, char *argv[])
{
	struct timespec t1, t2;

	wrs_msg_init(argc, argv);

	/* Print HAL's version */
	wrs_msg(LOG_ALERT, "wrsw_hal. Commit %s, built on " __DATE__ "\n",
		__GIT_VER__);

	/* Prevent from running HAL twice - it will likely freeze the system */
	if (hal_check_running()) {
		fprintf(stderr, "Fatal: There is another WR HAL "
			"instance running. We can't work together.\n\n");
		return -1;
	}

	hal_parse_cmdline(argc, argv);

	if (hal_init())
		exit(1);

	/*
	 * Main loop update - polls for WRIPC requests and rolls the port
	 * state machines. This is not a busy loop, as wripc waits for
	 * the "max ms delay". Unless an RPC call comes, in which
	 * case it returns earlier.
	 *
	 * We thus check the actual time, and only proceed with
	 * port and fan update every PORT_FAN_MS_PERIOD.  There still
	 * is some jitter from hal_update_wripc() timing.
	 * includes some jitter.
	 */

	clock_gettime(CLOCK_MONOTONIC, &t1);
	for (;;) {
		int delay_ms;

		hal_update_wripc(25 /* max ms delay */);

		clock_gettime(CLOCK_MONOTONIC, &t2);
		delay_ms = (t2.tv_sec - t1.tv_sec) * 1000;
		delay_ms += (t2.tv_nsec - t1.tv_nsec) / 1000 / 1000;
		if (delay_ms < PORT_FAN_MS_PERIOD)
			continue;

		hal_port_update_all();
		/* update fans and temperatures in shmem */
		shw_update_fans(&hal_shmem->temp);
		t1 = t2;
	}

	hal_shutdown();

	return 0;
}
