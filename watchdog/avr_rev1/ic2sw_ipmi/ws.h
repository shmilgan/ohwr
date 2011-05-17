/*
 * ws.h
 *
 *  Created on: 24-Jun-2009
 *      Author: poliveir
 */

#ifndef WS_H_
#define WS_H_

/* Working Set states */
#define WS_FREE				0x1	/* ws free */
#define WS_PENDING			0x2	/* ws not in any queue, ready for use */
#define WS_ACTIVE_IN			0x3	/* ws in incoming queue, ready for ipmi processing */
#define WS_ACTIVE_IN_PENDING		0x4	/* ws in use by the ipmi layer */
#define WS_ACTIVE_MASTER_WRITE		0x5	/* ws in outgoing queue */
#define WS_ACTIVE_MASTER_WRITE_PENDING	0x6	/* outgoing request in progress */
#define WS_ACTIVE_MASTER_WRITE_SUCCESS  0x7
#define WS_ACTIVE_MASTER_READ		0x8
#define WS_ACTIVE_MASTER_READ_PENDING	0x9

#define WS_ARRAY_SIZE	3

void ws_init( void );
IPMI_WS * ws_alloc( void );
void ws_set_state( IPMI_WS * ws, unsigned state );
IPMI_WS * ws_get_elem( unsigned state );
void ws_process_work_list( void );
void ws_free( IPMI_WS *ws );
void ws_process_incoming( IPMI_WS *ws );
IPMI_WS * ws_get_elem_seq( uint8_t seq, IPMI_WS *ws_ignore );


#endif /* WS_H_ */
