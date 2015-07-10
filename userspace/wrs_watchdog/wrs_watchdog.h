#ifndef __WRS_WATCHDOG__
#define __WRS_WATCHDOG__

static const char * const alloc_states[] = {
	"IDLE",
	"START_SETUCNT",
	"START_PGE_REQ",
	"INTER_PGE_REQ",
	"START_SET&REQ",
	"Unknown"};

static const char * const trans_states[] = {
	"IDLE",
	"READY",
	"WAIT_RTU_V",
	"WAIT_SOF",
	"SET_USECNT",
	"WAIT_W_TRANSFER",
	"TOO_LONG_TR",
	"TRANSFER",
	"TRANSFERRED",
	"DROP",
	"Unknown"};

static const char * const rcv_states[] = {
	"IDLE",
	"READY",
	"PAUSE",
	"RCV_DATA",
	"DROP",
	"WAIT_F_FREE",
	"INPUT_STUCK",
	"Unknown"};

static const char * const ll_states[] = {
	"IDLE",
	"RDY_PGR_&_DLAST",
	"RDY_DLAST_ONLY",
	"WRITE",
	"EOF_ON_WR",
	"SOF_ON_WR",
	"Unknown"};

static const char * const prep_states[] = {
	"RETRY_RDY",
	"NEWPCK_PG_RDY",
	"NEWPCK_PG_SET_ADV",
	"NEWPCK_PG_USED",
	"RETRY_PREPARE",
	"IDLE",
	"Unknown",
	"cycle frozen"};

static const char * const send_states[] = {
	"IDLE",
	"DATA",
	"FLUSH_STALL",
	"FINISH_CYCLE",
	"EOF",
	"RETRY",
	"WAIT_FREE_PCK",
	"Unknown"};

static const char * const free_states[] = {
	"IDLE",
	"REQ_RD_FIFO",
	"RD_FIFO",
	"RD_NEXT_PG_ADR",
	"FREE_CUR_PG_ADR",
	"F_FREE_CUR_PG_ADR",
	"Unknown"};

static const char idles[] = {
	0, /*alloc: s_idle */
	1, /* trans: s_ready */
	1, /* rcv:   s_ready */
	1, /* ll:    s_ready_for_pgr_and_dlast */
	5, /* prep:  s_idle */
	0, /* send:  s_idle */
	0  /* free:  s_idle */
};
#define FSMS_NO 7
#define ALLOC_IDX 0
#define TRANS_IDX 1
#define RCV_IDX   2
#define LL_IDX    3
#define PREP_IDX  4
#define SEND_IDX  5
#define FREE_IDX  6
struct swc_fsms {
	int state[FSMS_NO];
	char act[FSMS_NO];
};

#endif
