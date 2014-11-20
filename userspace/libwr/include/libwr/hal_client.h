
#ifndef __LIBWR_HAL_CLIENT_H
#define __LIBWR_HAL_CLIENT_H

#include <hal/hal_exports.h>

int halexp_client_init();
int halexp_client_try_connect(int retries, int timeout);

#endif /* __LIBWR_HAL_CLIENT_H */
