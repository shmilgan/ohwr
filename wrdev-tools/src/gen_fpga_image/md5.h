#ifndef __MD5_H
#define __MD5_H

#include <inttypes.h>

int md5_checksum(uint8_t *data, int length, uint8_t *digest);

#endif

