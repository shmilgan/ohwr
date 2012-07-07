#ifndef __NMEA_RESYNC_H
#define __NMEA_RESYNC_H

#include <time.h>

int nmea_get_unix_seconds(const char *dev_name, time_t *t_out);
void nmea_resync_ppsgen(const char *dev_name);

#endif
