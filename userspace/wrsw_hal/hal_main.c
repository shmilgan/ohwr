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

#include <libwr/trace.h>
#include <libwr/switch_hw.h>
#include <libwr/shw_io.h>
#include <libwr/sfp_lib.h>
#include <libwr/config.h>

#include "wrsw_hal.h"
#include <rt_ipc.h>

#define MAX_CLEANUP_CALLBACKS 16

#define assert_init(proc) { int ret; if((ret = proc) < 0) return ret; }

static int daemon_mode = 0;
static hal_cleanup_callback_t cleanup_cb[MAX_CLEANUP_CALLBACKS];

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
static void call_cleanup_cbs()
{
	int i;

	TRACE(TRACE_INFO, "Cleaning up...");
	for (i = 0; i < MAX_CLEANUP_CALLBACKS; i++)
		if (cleanup_cb[i])
			cleanup_cb[i] ();
}

static void sighandler(int sig)
{
	TRACE(TRACE_ERROR, "signal caught (%d)!", sig);

	//Set state led to orange
	shw_io_write(shw_io_led_state_o, 1);
	shw_io_write(shw_io_led_state_g, 0);

	call_cleanup_cbs();
	exit(0);
}

static int hal_shutdown()
{
	call_cleanup_cbs();
	return 0;
}

static void hal_deamonize();

/* Main initialization function */
static int hal_init()
{
	//trace_log_stderr();

	TRACE(TRACE_INFO, "HAL initializing...");

	memset(cleanup_cb, 0, sizeof(cleanup_cb));

	libwr_cfg_read_file("/wr/etc/dot-config"); /* FIXME: accept -f */
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

	assert_init(hal_init_timing());

	/* Initialize port FSMs and IPC/RPC - see hal_ports.c */
	assert_init(hal_port_init_all());

	//everything is fine up to here, we can blink green LED
	shw_io_write(shw_io_led_state_o, 0);
	shw_io_write(shw_io_led_state_g, 1);

	if (daemon_mode)
		hal_deamonize();

	return 0;
}

/* Turns a nice and well-behaving HAL into an evil servant of satan. */
static void hal_deamonize()
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

static void show_help()
{
	printf("WR Switch Hardware Abstraction Layer daemon (wrsw_hal)\n\
Usage: wrsw_hal [options], where [options] can be:\n\
-d       : fork into background (daemon mode)\n");
}

static void hal_parse_cmdline(int argc, char *argv[])
{
	int opt;

	while ((opt = getopt(argc, argv, "dh")) != -1) {
		switch (opt) {
		case 'd':
			daemon_mode = 1;
			break;

		case 'h':
			show_help();
			exit(0);
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

	trace_log_file("/dev/kmsg");
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
	#define PORT_FAN_MS_PERIOD 200

	clock_gettime(CLOCK_MONOTONIC, &t1);
	for (;;) {
		int delay_ms;

		hal_update_wripc(25 /* max ms delay */);

		clock_gettime(CLOCK_MONOTONIC, &t2);
		delay_ms = (t2.tv_sec - t1.tv_sec) * 1000;
		delay_ms += (t2.tv_nsec - t1.tv_nsec) / 1000 / 1000;
		if (delay_ms < PORT_FAN_MS_PERIOD)
			continue;

		/* update the "previous" stamp */
		t1.tv_nsec += PORT_FAN_MS_PERIOD * 1000 * 1000;
		if (t1.tv_nsec > 1000 * 1000 * 1000) {
			t1.tv_nsec -= 1000 * 1000 * 1000;
			t1.tv_sec++;
		}

		hal_port_update_all();
		shw_update_fans();
		t1 = t2;
	}

	hal_shutdown();

	return 0;
}
