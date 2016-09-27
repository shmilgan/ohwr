#ifndef __HIST_EXPORTS_H
#define __HIST_EXPORTS_H

#include <stdint.h>

#define WRS_HIST_SERVER_ADDR "wrs_hist"

#define HIST_SFP_INSERT 1
#define HIST_SFP_REMOVE 2

/* Export structures, shared by server and client for argument matching */
#ifdef HIST_EXPORT_STRUCTURES

struct minipc_pd hist_export_sfp_action = {
	.name = "sfp_action",
	.retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
	.args = {
		 MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int), /* action: add/remove */
		 MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRING, char *), /* vn */
		 MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRING, char *), /* pn */
		 MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRING, char *), /* sn */
		 MINIPC_ARG_END,
		 },
};

#endif /* HIST_EXPORT_STRUCTURES */

#endif /* __HIST_EXPORTS_H */
