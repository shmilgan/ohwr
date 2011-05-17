#include <stdio.h>
#include <stdlib.h>

#include <wr_ipc.h>
#include <wrsw_hal.h>

static wripc_handle_t hal_cli;

int halcli_check_running()
{
	int rval;
	wripc_call(hal_cli, "halexp_check_running", &rval, 0);
	return rval;
}

main()
{
	hal_cli = wripc_connect("wrsw_hal");
	if(hal_cli < 0)
	{
		printf("Unable to connect to HAL\n");
		return -1;
	}
	
	printf("HAL status: %s\n",  halcli_check_running() ? "running" : "inactive");
}
