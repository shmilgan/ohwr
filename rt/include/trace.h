#ifndef __FREESTANDING_TRACE_H__
#define __FREESTANDING_TRACE_H__
#include <pp-printf.h>

#define TRACE(...) pp_printf(__VA_ARGS__)
#define TRACE_DEV(...) pp_printf(__VA_ARGS__)

#endif
