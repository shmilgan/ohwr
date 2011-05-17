/*
 * ws.c
 *
 *  Created on: 24-Jun-2009
 *      Author: poliveir
 */
#include <avr/pgmspace.h>
#include <stdio.h>
#include "ipmi.h"
#include "uart.h"
#include "ws.h"
#include "i2csw.h"

extern unsigned long lbolt;

IPMI_WS	ws_array[WS_ARRAY_SIZE];

/* initialize ws structures */
void
ws_init( void )
{
	unsigned i;

	for ( i = 0; i < WS_ARRAY_SIZE; i++ )
	{
		ws_array[i].ws_state = WS_FREE;
	}

}

/* get a free ws elem */
IPMI_WS *
ws_alloc( void )
{
	IPMI_WS *ws = 0;
	IPMI_WS *ptr = ws_array;
	unsigned i;
	//unsigned int interrupt_mask = CURRENT_INTERRUPT_MASK;

	//DISABLE_INTERRUPTS;
	for ( i = 0; i < WS_ARRAY_SIZE; i++ )
	{
		ptr = &ws_array[i];
		if( ptr->ws_state == WS_FREE ) {
			ptr->ws_state = WS_PENDING;
			ws = ptr;
			break;
		}
	}
	//ENABLE_INTERRUPTS( interrupt_mask );
	return ws;
}

void ws_set_state( IPMI_WS * ws, unsigned state )
{
	ws->ws_state = state;
	ws->timestamp = lbolt;
}

/*==============================================================
 * ws_process_work_list()
 * 	Go through the active list, calling the ipmi handler for
 * 	incoming entries and transport handler for outgoing entries.
 *==============================================================*/
void ws_process_work_list( void )
{
	IPMI_WS *ws;

	ws = ws_get_elem( WS_ACTIVE_IN );
	if( ws ) {
		printfr(PSTR("ws_process_work_list: found a WS_ACTIVE_IN\n\r"));
		ws_set_state( ws, WS_ACTIVE_IN_PENDING );
		// ipmi_process_pkt( ws );
		if( ws->ipmi_completion_function )
		{
			( ws->ipmi_completion_function )( (void *)ws, XPORT_REQ_NOERR );
		}
		else
		{
			printfr(PSTR("ws_process_work_list: ws_process_incoming\n\r"));
			ws_process_incoming( ws ); // otherwise call the default handler
		}
	}
	ws = ws_get_elem( WS_ACTIVE_MASTER_WRITE );
	if( ws ) {
		ws_set_state( ws, WS_ACTIVE_MASTER_WRITE_PENDING );

		switch( ws->outgoing_medium ) {
			case IPMI_CH_MEDIUM_IPMB:
				i2c_master_write( ws );
				break;

			case IPMI_CH_MEDIUM_SERIAL:	/* Asynch. Serial/Modem (RS-232) 	*/
				//serial_tm_send( ( unsigned char * )ws );
				break;

			case IPMI_CH_MEDIUM_ICMB10:	/* ICMB v1.0 				*/
			case IPMI_CH_MEDIUM_ICMB09:	/* ICMB v0.9 				*/
			case IPMI_CH_MEDIUM_LAN:	/* 802.3 LAN 				*/
			case IPMI_CH_MEDIUM_LAN_AUX:	/* Other LAN				*/
			case IPMI_CH_MEDIUM_PCI_SMB:	/* PCI SMBus				*/
			case IPMI_CH_MEDIUM_SMB_1x:	/* SMBus v1.0/1.1			*/
			case IPMI_CH_MEDIUM_SMB_20:	/* SMBus v2.0				*/
			case IPMI_CH_MEDIUM_USB_1x:	/* reserved for USB 1.x			*/
			case IPMI_CH_MEDIUM_USB_20:	/* reserved for USB 2.x			*/
			case IPMI_CH_MEDIUM_SYS:	/* System Interface (KCS, SMIC, or BT)	*/
				printfr(PSTR("ws_process_work_list: unsupported protocol\n\r"));
				ws_free( ws );
				break;
		}
	}

	ws = ws_get_elem( WS_ACTIVE_MASTER_READ );
	if( ws ) {
		ws_set_state( ws, WS_ACTIVE_MASTER_READ_PENDING );
		if( ws->incoming_protocol == IPMI_CH_PROTOCOL_IPMB )
			i2c_master_read( ws );
	}
}

IPMI_WS *
ws_get_elem( unsigned state )
{
	IPMI_WS *ws = 0;
	IPMI_WS *ptr = ws_array;
	unsigned i;
	//unsigned int interrupt_mask = CURRENT_INTERRUPT_MASK;

	//DISABLE_INTERRUPTS;
	for ( i = 0; i < WS_ARRAY_SIZE; i++ )
	{
		ptr = &ws_array[i];
		if( ptr->ws_state == state ) {
			if( ws ) {
				if( ptr->timestamp < ws->timestamp )
					ws = ptr;
			} else {
				ws = ptr;
			}
		}
	}

	if( ws )
	 ws->timestamp = lbolt;

	//ENABLE_INTERRUPTS( interrupt_mask );
	return ws;
}

/* set ws state to free */
void ws_free( IPMI_WS *ws )
{
	int len, i;
	char *ptr = (char *)ws;

	len = sizeof( IPMI_WS );
	for( i = 0 ; i < len ; i++ ) {
		*ptr++ = 0;
	}
	ws->incoming_protocol = IPMI_CH_PROTOCOL_NONE;
	ws->ws_state = WS_FREE;
}

/* Default handler for incoming packets */
void ws_process_incoming( IPMI_WS *ws )
{
	ipmi_process_pkt( ws );
	return;
}
IPMI_WS * ws_get_elem_seq( uint8_t seq, IPMI_WS *ws_ignore )
{
	IPMI_WS *ws = 0;
	IPMI_WS *ptr = ws_array;
	unsigned i;
	//unsigned int interrupt_mask = CURRENT_INTERRUPT_MASK;

	//DISABLE_INTERRUPTS;
	for ( i = 0; i < WS_ARRAY_SIZE; i++ )
	{
		ptr = &ws_array[i];
		if( ptr == ws_ignore )
			continue;
		if( ( ptr->ws_state != WS_FREE ) && ( ptr->seq_out == seq ) ) {
			ws = ptr;
			break;
		}
	}
	//ENABLE_INTERRUPTS( interrupt_mask );
	return ws;
}
