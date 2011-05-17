/*
 * ipmi.c
 *
 *  Created on: 20-Jun-2009
 *      Author: poliveir
 */
#include <avr/pgmspace.h>
#include "uart.h"
#include "ipmi.h"
#include "event.h"
#include "i2csw.h"
#include "module.h"
#include "ws.h"
#include "string.h"
#include <stdio.h>

uint8_t ipmi_calculate_checksum( uint8_t *ptr, uint8_t numchar )
{
	uint8_t checksum = 0;
	uint8_t i;

	for( i = 0; i < numchar; i++ ) {
		checksum += *ptr++;
	}
	return  (-checksum);
}
uint8_t seq_array[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
/* sequence number generator */
uint8_t ipmi_get_next_seq( uint8_t *seq )
{
	unsigned short i;

	/* return the first free sequence number */
	for( i = 0; i < 16; i++ ) {
		if( !seq_array[i] ) {
			seq_array[i] = 1;
			*seq = i;
			return( 1 );
		}
	}
	return( 0 );
}
void
ipmi_process_pkt( IPMI_WS * ws )
{
	IPMI_PKT	*pkt;
	uint8_t		cksum, completion_code = CC_NORMAL;
	uint8_t		responder_slave_addr;//, requester_slave_addr;

	IPMI_IPMB_HDR *ipmb_hdr = ( IPMI_IPMB_HDR * )&( ws->pkt_in );

	printfr(PSTR("\n\ripmi_process_pkt: ingress\n\r"));

	pkt = &ws->pkt;
	pkt->hdr.ws = (uint8_t *)ws;

	switch( ws->incoming_protocol ) {
		case IPMI_CH_PROTOCOL_IPMB:	/* used for IPMB, serial/modem Basic Mode, and LAN */
			printfr(PSTR("ipmi_process_pkt: IPMB protocol\n\r"));

			/* this is an interface that includes checksums -> verify payload integrity */
			/* need to check both header_checksum & data_checksum for requests. A response
			 * is directly bridged an we let the original requester worry about checksums */

			if( !( ipmb_hdr->netfn % 2 ) ) {
				/* an even netfn indicates a request */
				responder_slave_addr = ws->pkt_in[0];/* TODO: write function to get the responder address*/
				cksum = -(  ws->pkt_in[0]  + ws->pkt_in[1] );
				if( ws->pkt_in[2] != cksum )
				{
					/* header checksum is the second byte */
					printfr(PSTR("ipmi_process_pkt: Faulty header checksum\n\r"));
					completion_code = CC_INVALID_DATA_IN_REQ;
					ws->len_out=0;
					break;
				}

				cksum = ipmi_calculate_checksum( &(((IPMI_IPMB_REQUEST *)(ws->pkt_in))->requester_slave_addr),
						ws->len_in - 4 );
				if( ws->pkt_in[ws->len_in - 1] != cksum ) { /* data checksum is the last byte */
					printfr(PSTR("ipmi_process_pkt: Faulty data checksum\n\r"));
					completion_code = CC_INVALID_DATA_IN_REQ;
					ws->len_out=0;
					break;
				}
			}

			/* fill in the IPMI_PKT structure */
			{
				IPMI_IPMB_REQUEST *ipmb_req;
				IPMI_IPMB_RESPONSE *ipmb_resp;

				if( ipmb_hdr->lun == 2 ) {
					/* route this to the system interface without processing ) */
				}

				if( ipmb_hdr->netfn % 2 ) {
					/* an odd netfn indicates a response */
					ipmb_req = NULL;
					ipmb_resp = ( IPMI_IPMB_RESPONSE * )&( ws->pkt_in );
					pkt->hdr.resp_data_len = ws->len_in - 8;
				} else {
					/* an even netfn is a request */
					ipmb_req = ( IPMI_IPMB_REQUEST * )&( ws->pkt_in );
					ipmb_resp = ( IPMI_IPMB_RESPONSE * )&( ws->pkt_out );
					pkt->hdr.responder_lun = ipmb_req->responder_lun;
					pkt->req = ( IPMI_CMD_REQ * )&( ipmb_req->command );
					pkt->hdr.req_data_len = ws->len_in - 7;
				}

				pkt->resp = ( IPMI_CMD_RESP * )&( ipmb_resp->completion_code );
				pkt->hdr.netfn = ipmb_hdr->netfn;

				/* check if responder_lun is valid */
				if( ipmb_req->responder_lun + 1 > NUM_LUN )
					completion_code = CC_INVALID_CMD;
			}

			break;


		default:
			printfr(PSTR("ipmi_process_pkt: unsupported protocol\n\r"));
			completion_code = CC_NOT_SUPPORTED;
			break;
	} /* end switch(incoming_protocol) */

	if( pkt->hdr.netfn % 2 )
	{
		/* an odd netfn indicates a response */
		ipmi_process_response( pkt, completion_code );
		printfr(PSTR("ipmi_process_response: egress\n"));
		return;
	}

	if( completion_code == CC_NORMAL )
		ipmi_process_request( pkt );

	/* ipmi_process_request fills in the |completion_code|data| portion
	 * and also sets pkt->hdr.resp_data_len */

	/* send back response */
	if( completion_code != CC_DELAYED_COMPLETION && completion_code != CC_INVALID_DATA_IN_REQ )
	{
		ws->outgoing_protocol = ws->incoming_protocol;
		ws->outgoing_medium = ws->incoming_medium;
		switch( ws->outgoing_protocol ) {
			case IPMI_CH_PROTOCOL_IPMB: {
				IPMI_IPMB_RESPONSE *ipmb_resp = ( IPMI_IPMB_RESPONSE * )&( ws->pkt_out );
				IPMI_IPMB_REQUEST *ipmb_req = ( IPMI_IPMB_REQUEST * )&( ws->pkt_in );


				ipmb_resp->requester_slave_addr = ipmb_req->requester_slave_addr;
				ipmb_resp->netfn = ipmb_req->netfn + 1;
				ipmb_resp->requester_lun = ipmb_req->requester_lun;
				ipmb_resp->header_checksum = -( ipmb_resp->requester_slave_addr + *(( char * )(ipmb_resp)+1));
				ipmb_resp->responder_slave_addr = module_get_i2c_address( I2C_ADDRESS_LOCAL );//TODO
				ipmb_resp->req_seq = ipmb_req->req_seq;
				ipmb_resp->responder_lun = ipmb_req->responder_lun;
				ipmb_resp->command = ipmb_req->command;
				ipmb_resp->completion_code=completion_code;

				/*
				 * The location of data_checksum field is bogus.
				 * It's used as a placeholder to indicate that a checksum follows the data field.
				 * The location of the data_checksum depends on the size of the data preceeding it.
				 * */
				ipmb_resp->data_checksum =
											ipmi_calculate_checksum( &ipmb_resp->responder_slave_addr,
																		pkt->hdr.resp_data_len + 4 );
				ws->len_out = sizeof(IPMI_IPMB_RESPONSE)
					- IPMB_RESP_MAX_DATA_LEN  +  pkt->hdr.resp_data_len;
				/* Assign the checksum to it's proper location */
				*( (uint8_t *)ipmb_resp + ws->len_out - 1 ) = ipmb_resp->data_checksum;
				}
				ws_set_state( ws, WS_ACTIVE_MASTER_WRITE );
				break;

			case IPMI_CH_PROTOCOL_TMODE: {		/* Terminal Mode */
			case IPMI_CH_PROTOCOL_ICMB:		/* ICMB v1.0 */
			case IPMI_CH_PROTOCOL_SMB:		/* IPMI on SMSBus */
			case IPMI_CH_PROTOCOL_KCS:		/* KCS System Interface Format */
			case IPMI_CH_PROTOCOL_SMIC:		/* SMIC System Interface Format */
			case IPMI_CH_PROTOCOL_BT10:		/* BT System Interface Format, IPMI v1.0 */
			case IPMI_CH_PROTOCOL_BT15:		/* BT System Interface Format, IPMI v1.5 */
				/* we should not be here  */
				printfr(PSTR("ipmi_process_pkt: unsupported protocol\n" ));
				break;
			}
		}


	}

	/* If completion code is CC_DELAYED_COMPLETION do nothing */

	printfr(PSTR("ipmi_process_pkt: egress\n"));
}

/* firmware transfer request */
void ipmi_process_fw_req( IPMI_PKT *pkt )
{
	printfr(PSTR("ipmi_process_fw_req: ingress\n"));
	pkt->resp->completion_code = CC_INVALID_CMD;
	pkt->hdr.resp_data_len = 0;
}

void ipmi_default_response( IPMI_PKT *pkt )
{
	printfr(PSTR("ipmi_default_response: ingress\n"));
	pkt->resp->completion_code = CC_INVALID_CMD;
	pkt->hdr.resp_data_len = 0;
}


void ipmi_process_request( IPMI_PKT *pkt )
{
	printfr(PSTR("ipmi_process_request: ingress\n\r"));

	switch( pkt->hdr.netfn ) {
		case NETFN_GROUP_EXTENSION_REQ:
			printfr(PSTR("ipmi_process_request: NETFN_GROUP_EXTENSION_REQ\n\r"));
		//	picmg_process_command( pkt );
			break;
		case NETFN_APP_REQ:
			printfr(PSTR("ipmi_process_request: NETFN_APP_REQ\n\r"));
		//	ipmi_process_app_req( pkt );
			break;
		case NETFN_EVENT_REQ:
			printfr(PSTR( "ipmi_process_request: NETFN_EVENT_REQ\n\r"));
			ipmi_process_event_req( pkt );
			break;
		case NETFN_FW_REQ:
			printfr(PSTR("ipmi_process_request: NETFN_FW_REQ\n\r"));
			ipmi_process_fw_req( pkt );
			break;
		case NETFN_NVSTORE_REQ:
			printfr(PSTR("ipmi_process_request: NETFN_NVSTORE_REQ\n\r"));
		//	ipmi_process_nvstore_req( pkt );
			break;
		default:
			printfr(PSTR("ipmi_process_request: default\n\r"));
			ipmi_default_response( pkt );
			break;
	}

	printfr(PSTR("ipmi_process_request: egress\n\r"));
}

void ipmi_process_response( IPMI_PKT *pkt, uint8_t completion_code )
{
	IPMI_WS *req_ws = 0,*resp_ws = 0,*target_ws=0;
	uint8_t seq = 0;

	printfr(PSTR("ipmi_process_response: ingress\n\r"));

	resp_ws = ( IPMI_WS * )pkt->hdr.ws;
	if( resp_ws->incoming_protocol == IPMI_CH_PROTOCOL_IPMB )
	{
		seq = ((IPMI_IPMB_REQUEST *)(resp_ws->pkt_in))->req_seq;

		/* using the seq#, check to see if there is an outstanding target request ws
		 * corresponding to this response */
		target_ws = ws_get_elem_seq( seq, resp_ws );

	}
	else {
		/* TODO: currently unsupported */
		/* in instances where seq number is not used then the interface is waiting
		 * for the command to complete and there is a single outstanding ws */
		// target_ws = bridging_ws;
	}

	if( !target_ws )
	 {
		//call module response handler here
		module_process_response( req_ws, seq, completion_code );

#ifdef DUMP_RESPONSE
		putstr( "\n[" );
		for( i = 0; i < resp_ws->len_in; i++ ) {
			puthex( resp_ws->pkt_in[i] );
			putchar( ' ' );
		}
		putstr( "]\n" );
#endif

		ws_free( resp_ws );
		return;
	} else {
		req_ws = target_ws->bridged_ws;
	}

	if( !req_ws ) {
		ws_free( resp_ws );
		ws_free( target_ws );
		return;
	}

	printfr(PSTR("response message not found forward to other ports\n\r"));

	memcpy( req_ws->pkt.resp, target_ws->pkt.resp, WS_BUF_LEN );
   // TODO: make sure pkt-> pointers are set properly
	req_ws->len_out = target_ws->len_in;

	ws_free( resp_ws );
	ws_free( target_ws );

	/* send back response */
	req_ws->outgoing_protocol = req_ws->incoming_protocol;
	req_ws->outgoing_medium = req_ws->incoming_medium;

	switch( req_ws->outgoing_protocol ) {
		case IPMI_CH_PROTOCOL_IPMB: {
			IPMI_IPMB_RESPONSE *ipmb_resp = ( IPMI_IPMB_RESPONSE * )&( req_ws->pkt_out );
			IPMI_IPMB_REQUEST *ipmb_req = ( IPMI_IPMB_REQUEST * )&( req_ws->pkt_in );

			ipmb_resp->requester_slave_addr = ipmb_req->requester_slave_addr;
			ipmb_resp->netfn = ipmb_req->netfn + 1;
			ipmb_resp->requester_lun = ipmb_req->requester_lun;
			ipmb_resp->header_checksum =-( ipmb_resp->requester_slave_addr + *(( char * )(ipmb_resp)+1));
			ipmb_resp->responder_slave_addr = module_get_i2c_address( I2C_ADDRESS_LOCAL );
			ipmb_resp->req_seq = ipmb_req->req_seq;
			ipmb_resp->responder_lun = ipmb_req->responder_lun;
			ipmb_resp->command = ipmb_req->command;
			/* The location of data_checksum field is bogus.
			 * It's used as a placeholder to indicate that a checksum follows the data field.
			 * The location of the data_checksum depends on the size of the data preceeding it.*/
			ipmb_resp->data_checksum =
				ipmi_calculate_checksum( &ipmb_resp->responder_slave_addr,
					pkt->hdr.resp_data_len + 4 );
			req_ws->len_out = sizeof(IPMI_IPMB_RESPONSE)
				- IPMB_RESP_MAX_DATA_LEN  +  pkt->hdr.resp_data_len;
			/* Assign the checksum to it's proper location */
			*( (uint8_t *)ipmb_resp + req_ws->len_out) = ipmb_resp->data_checksum;
			}
			break;

		case IPMI_CH_PROTOCOL_TMODE: {		/* Terminal Mode */
		case IPMI_CH_PROTOCOL_ICMB:		/* ICMB v1.0 */
		case IPMI_CH_PROTOCOL_SMB:		/* IPMI on SMSBus */
		case IPMI_CH_PROTOCOL_KCS:		/* KCS System Interface Format */
		case IPMI_CH_PROTOCOL_SMIC:		/* SMIC System Interface Format */
		case IPMI_CH_PROTOCOL_BT10:		/* BT System Interface Format, IPMI v1.0 */
		case IPMI_CH_PROTOCOL_BT15:		/* BT System Interface Format, IPMI v1.5 */
			/* Unsupported protocol */
			printfr(PSTR("ipmi_process_pkt: unsupported protocol\n"));
			break;
	}
	ws_set_state( req_ws, WS_ACTIVE_MASTER_WRITE );
	}
}






