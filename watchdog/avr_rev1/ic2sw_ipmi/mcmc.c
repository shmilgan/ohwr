/*
 * mcmc.c
 *
 *  Created on: 24-Jun-2009
 *      Author: poliveir
 */
#include <string.h>
#include <avr/pgmspace.h>
#include "ipmi.h"
#include "uart.h"
#include "module.h"
#include "i2csw.h"
#include "ws.h"
#include <stdio.h>
#include "amc.h"
#include "tm100.h"

void module_init2( void );
void mcmc_mmc_event( uint8_t dev_id, uint8_t event );
void enable_payload( uint8_t dev_id );

uint8_t lookup_dev_id( uint8_t dev_addr );
uint8_t  lookup_dev_addr( uint8_t dev_id );

uint8_t g_dev_id = 0xff;	// dev id used for debugging & console commands

#define IPMBL_TABLE_SIZE	27
#define NUM_SLOTS		IPMBL_TABLE_SIZE

uint8_t IPMBL_TABLE[IPMBL_TABLE_SIZE] = {
	0x70, 0x8A, 0x72, 0x8E, 0x92, 0x90, 0x74, 0x8C, 0x76, 0x98, 0x9C,
	0x9A, 0xA0, 0xA4, 0x88, 0x9E, 0x86, 0x84, 0x78, 0x94, 0x7A, 0x96,
	0x82, 0x80, 0x7C, 0x7E, 0xA2 };


#define NUM_AMC_SLOTS	1

#define MAX_SDR_DATA	5
#define MAX_FRU_DATA	2
#define MAX_SDR_COUNT		8
#define MAX_SENSOR_COUNT	8

typedef struct _sdr_data {
	uint8_t record_id;
	uint8_t record_len;
	uint8_t sdr[MAX_SDR_DATA];
} SDR_DATA;
/*
FULL_SENSOR_RECORD sdr3 = {
	{ 0,0 },	// record_id[0-1] of this record
	0x51,		// sdr_version;
	0x1,		// record_type = 1 - Full sensor record
	0x30,		// record_len - Number of remaining record bytes following
	0,		   // dev_slave_addr - [7:1] - 7-bit I2C Slave Address of device on channel. Fill during init.
	1,		// ch_num - [3:0] - Channel number for the channel that the management controller is on.

	// BYTE 6
	0,		// owner_id - 7-bit I2C Slave, fill during init
	0,		// id_type - 0b = owner_id is IPMB Slave Address

	// BYTE 7
	0,		// [7:4] channel_num
	0,		// [3:2] reserved
	0,		// [1:0] sensor_owner_lun

	0x10,		// 8 sensor number
	0xC1,		// 9 entity_id

	// BYTE 10
	0,		// entity_type - 0b = treat entity as a physical entity
	0x68,		// entity_instance_num - 60h-7Fh device-relative Entity Instance.

	// BYTE 11 - Sensor initialization
	0,		// [7] - reserved. Write as 0b
	0,		// [6] init_scanning
	0,		// [5] init_events
	0,		// [4] - reserved. Write as 0b
	0,		// [3] init_hysteresis
	0,		// [2] init_sensor_type
	1,		// [1] powerup_evt_generation
	1,		// [0] powerup_sensor_scanning

	// BYTE 12 - Sensor capabilities
	0,		// [7] ignore_sensor
	1,		// [6] sensor_manual_support
	3,		// [5:4] sensor_hysteresis_support
	2,		// [3:2] sensor_threshold_access
	0,		// [1:0] event_msg_control

	0x1,		// 13 sensor_type = 1 - Temp sensor
	0x1,		// 14 event_type_code

	0x8002,		// 15,16 assertion event_mask

	0x8032,		// 17,18 deassertion event mask
	0x3F3F,		// 19,20 reading_mask

	// BYTE 21
	2,		// [7:6] Analog (numeric) Data Format - 2’s complement (signed)
	0,		// [5:3] rate_unit - 000b = none
	0,		// [2:1] modifier_unit - 00b = none
	0,		// [0] percentage - 0b

	1,		// 22 sensor_units2
	0,		// 23 sensor_units3

	0,		// 24 linearization
	1,		// 25 M
	0,		// 26 M Tolerance
	0,		// 27 B
	0,		// 28 B Accuracy
	0,		// 29 Accuracy, Accuracy exp, Sensor Direction
	0,		// 30 R exp, B exp
	0,		// 31 Analog characteristic flags
	0,		// 32 Nominal Reading
	0,		// 33 Normal Maximum - Given as a raw value.
	0,		// 34 Normal Minimum - Given as a raw value.
	0x7F,		// 35 Sensor Maximum Reading
	0xC9,		// 36 Sensor Minimum Reading
	0x7F,		// 37 Upper non-recoverable Threshold
	0x4B,		// 38 Upper critical Threshold
	0x3C,		// 39 Upper non-critical Threshold
	0xFB,		// 40 Lower non-recoverable Threshold
	5,		// 41 Lower critical Threshold
	0xA,		// 42 Lower non-critical Threshold
	2,		// 43 Positive-going Threshold Hysteresis value
	2,		// 44 Negative-going Threshold Hysteresis value
	0,		// 45 reserved. Write as 00h.
	0,		// 46 reserved. Write as 00h.
	0,		// 47 OEM - Reserved for OEM use.
	0xC5,		// 48 id_str_typ_len Sensor ID String Type/Length Code, 5 chars in str
	{ 'T', 'E', 'M', 'P', '1' }		// sensor_id_str[]
};
*/

typedef struct _fru_data {
	uint8_t	fru_dev_id;
	unsigned int	fru_inventory_area_size;
	uint8_t	fru[MAX_FRU_DATA];
} FRU_DATA;

// per AMC data structure, gets filled during device discovery
typedef struct _amc_info {
	uint8_t 			state;
	GET_DEVICE_ID_CMD_RESP		device_id;
	GET_PICMG_PROPERTIES_CMD_RESP 	picmg_properties;
	GET_DEVICE_SDR_INFO_RESP 	sdr_info;
	uint8_t			sdr_to_process;
	SDR_DATA			sdr_data[MAX_SDR_COUNT];
	GET_FRU_INVENTORY_AREA_CMD_RESP	fru_info;
	FRU_CONTROL_CAPABILITIES_CMD_RESP	fru_capabilities;
	unsigned short			current_fru_offset;
	uint8_t			current_record_len;
	uint8_t			current_read_len;
	uint8_t			record_read;
	uint8_t			fru_read_state;
	FRU_DATA			fru_data;
	GET_CLOCK_STATE_CMD_RESP	clock_state;
	GET_AMC_PORT_STATE_CMD_RESP	port_state;
} AMC_INFO;

uint8_t mcmc_ipmbl_address;
AMC_INFO amc[NUM_AMC_SLOTS];

/*
ATCA_CMD_SET_FRU_LED_STATE
ATCA_CMD_FRU_CONTROL
*/
#define LED_OFF			0
#define LED_ON			1
#define LED_LONG_BLINK		2
#define LED_SHORT_BLINK		3

void send_set_fru_led_state( IPMI_WS *req_ws, uint8_t ipmi_ch, uint8_t dev_addr, uint8_t led_state, void( *completion_function )( IPMI_WS *, int ) )
{
	IPMI_PKT *pkt;

	uint8_t seq;
	uint8_t responder_slave_addr;
	SET_FRU_LED_STATE_CMD_REQ *req;
	IPMI_IPMB_REQUEST *ipmb_req;

	/*if( !( req_ws = ws_alloc() ) ) {
		return;
	}
	 */
	pkt = &( req_ws->pkt );
	ipmb_req = ( IPMI_IPMB_REQUEST * )&( req_ws->pkt_out );
	pkt->req = ( IPMI_CMD_REQ * )&( ipmb_req->command ) ;
	req = ( SET_FRU_LED_STATE_CMD_REQ * )pkt->req;
	ipmi_get_next_seq( &seq );

	pkt->hdr.ws = (uint8_t *)req_ws;
	pkt->hdr.req_data_len = sizeof( SET_FRU_LED_STATE_CMD_REQ ) - 1;

	req->command = ATCA_CMD_SET_FRU_LED_STATE;
	req->picmg_id = 0;
	req->fru_dev_id = 0;
	req->led_id = FRU_LED_BLUE;
	switch( led_state ) {
		case LED_OFF:
			req->led_function = 0;
			break;
		case LED_ON:
			req->led_function = 0xff;
			break;
		case LED_LONG_BLINK:
			req->led_function = 200;	// off duration in tens of ms
			req->on_duration = 200;		// tens of ms
			break;
		case LED_SHORT_BLINK:
			req->led_function = 50;		// off duration in tens of ms
			req->on_duration = 50;		// tens of ms
			break;
		default:
			break;
	}
	req->color = 0xff;		// use default color

	ipmb_req->requester_slave_addr = module_get_i2c_address( I2C_ADDRESS_LOCAL );
	ipmb_req->netfn = NETFN_GROUP_EXTENSION_REQ;
	ipmb_req->requester_lun = 0;
	responder_slave_addr = dev_addr;
	ipmb_req->header_checksum = -( *( char * )ipmb_req + responder_slave_addr );
	ipmb_req->req_seq = seq;
	ipmb_req->responder_lun = 0;
	ipmb_req->command = req->command;
	/* The location of data_checksum field is bogus.
	 * It's used as a placeholder to indicate that a checksum follows the data field.
	 * The location of the data_checksum depends on the size of the data preceeding it.*/
	ipmb_req->data_checksum =
		ipmi_calculate_checksum( &( ipmb_req->requester_slave_addr ),
			pkt->hdr.req_data_len + 3 );
	req_ws->len_out = sizeof( IPMI_IPMB_REQUEST )
				- IPMB_REQ_MAX_DATA_LEN  +  pkt->hdr.req_data_len;
	/* Assign the checksum to it's proper location */
	*( ( uint8_t * )ipmb_req + req_ws->len_out - 1 ) = ipmb_req->data_checksum;

	//req_ws->ipmi_completion_function = completion_function;
	req_ws->addr_out = dev_addr;
	req_ws->outgoing_protocol = IPMI_CH_PROTOCOL_IPMB;
	req_ws->outgoing_medium = IPMI_CH_MEDIUM_IPMB;
	req_ws->outgoing_channel = ipmi_ch;
	//TODO dump_outgoing( req_ws );

	/* dispatch the request */
	//TODO ws_set_state( req_ws, WS_ACTIVE_MASTER_WRITE );
}

void send_picmg_properties(IPMI_WS *req_ws, uint8_t ipmi_ch, uint8_t dev_addr)
{
	IPMI_PKT *pkt;

	uint8_t seq;

	GET_PICMG_PROPERTIES_CMD_REQ *req;
	IPMI_IPMB_REQUEST *ipmb_req;

	pkt = &( req_ws->pkt );
	ipmb_req = ( IPMI_IPMB_REQUEST * )&( req_ws->pkt_out );
	pkt->req = ( IPMI_CMD_REQ * )&( ipmb_req->command ) ;
	req = ( GET_PICMG_PROPERTIES_CMD_REQ * )pkt->req;
	ipmi_get_next_seq( &seq );

	pkt->hdr.ws = (uint8_t *)req_ws;
	pkt->hdr.req_data_len = sizeof( GET_PICMG_PROPERTIES_CMD_REQ ) - 1;

	req->command = ATCA_CMD_GET_PICMG_PROPERTIES;
	req->picmg_id=0x00;
	ipmb_req->requester_slave_addr = module_get_i2c_address( I2C_ADDRESS_LOCAL );
	ipmb_req->netfn = NETFN_PICMG_REQ;
	ipmb_req->requester_lun = 0;
	ipmb_req->responder_slave_addr = dev_addr;
	ipmb_req->header_checksum = -(ipmb_req->requester_slave_addr + *(( char * )(ipmb_req+1)));
	ipmb_req->req_seq = 0x05;
	ipmb_req->responder_lun = 0;
	ipmb_req->command = req->command;
	/* The location of data_checksum field is bogus.
	 * It's used as a placeholder to indicate that a checksum follows the data field.
	 * The location of the data_checksum depends on the size of the data preceeding it.*/
	ipmb_req->data_checksum =
		ipmi_calculate_checksum( &ipmb_req->requester_slave_addr,
			pkt->hdr.req_data_len + 3 );
	req_ws->len_out = sizeof( IPMI_IPMB_REQUEST )
				- IPMB_REQ_MAX_DATA_LEN  +  pkt->hdr.req_data_len;
	/* Assign the checksum to it's proper location */
	*( ( uint8_t * )ipmb_req + req_ws->len_out - 1 ) = ipmb_req->data_checksum;

	//req_ws->ipmi_completion_function = completion_function; //TODO: add completion function
	req_ws->addr_out = dev_addr;
	req_ws->outgoing_protocol = IPMI_CH_PROTOCOL_IPMB;
	req_ws->outgoing_medium = IPMI_CH_MEDIUM_IPMB;
	req_ws->outgoing_channel = ipmi_ch;
	//dump_outgoing( req_ws );

	/* dispatch the request */
    //ws_set_state( req_ws, WS_ACTIVE_MASTER_WRITE );
}


/*
 * cmd_complete()
 *
 * Completion function
 */
void
cmd_complete( IPMI_WS *ws, int status )
{
	switch( status ) {
		case XPORT_REQ_NOERR:
		default:
			//TODO ws_free( ws );
			printfr(PSTR("\n\rcommand successfully sent\n\r"));
			break;
	}
}


/*==============================================================
 * MCMC ADDRESSING
 *==============================================================*/
uint8_t module_get_i2c_address( int address_type )
{
	switch( address_type ) {
		case I2C_ADDRESS_LOCAL:
			return 0x20;
			break;
		case I2C_ADDRESS_REMOTE:	// used for debugging
			if( g_dev_id == 0xff ) {
				return 0x9c;
			} else {
				return ( lookup_dev_addr( g_dev_id ) );
			}
			break;
		default:
			return 0;
	}
}

/* given the slot number, lookup the i2c addr */
uint8_t lookup_dev_addr( uint8_t dev_id )
{
	if( dev_id < IPMBL_TABLE_SIZE )	// assuming slot enumeration starts from 0, TODO check
		return( IPMBL_TABLE[dev_id] );
	else
		return 0xff;
}

/* given the i2c addr lookup the slot number */
uint8_t lookup_dev_id( uint8_t dev_addr )
{
	int i;

	for( i = 0; i < IPMBL_TABLE_SIZE ; i++ ) {
		if( IPMBL_TABLE[i] == dev_addr )
			return i;
	}
	return 0xff;
}




void send_get_device_id(IPMI_WS *req_ws, uint8_t ipmi_ch, uint8_t dev_addr)
{
	IPMI_PKT *pkt;

	uint8_t seq;
	uint8_t responder_slave_addr;
	GENERIC_CMD_REQ *req;
	IPMI_IPMB_REQUEST *ipmb_req;

	pkt = &( req_ws->pkt );
	ipmb_req = ( IPMI_IPMB_REQUEST * )&( req_ws->pkt_out );
	pkt->req = ( IPMI_CMD_REQ * )&( ipmb_req->command ) ;
	req = ( GENERIC_CMD_REQ * )pkt->req;
	ipmi_get_next_seq( &seq );

	pkt->hdr.ws = (uint8_t *)req_ws;
	pkt->hdr.req_data_len = sizeof( GENERIC_CMD_REQ ) - 1;

	req->command = IPMI_CMD_GET_DEVICE_ID;

	ipmb_req->requester_slave_addr = module_get_i2c_address( I2C_ADDRESS_LOCAL );
	ipmb_req->netfn = NETFN_APP_REQ;
	ipmb_req->requester_lun = 0;
	responder_slave_addr = dev_addr;
	ipmb_req->header_checksum = -(ipmb_req->requester_slave_addr + *(( char * )(ipmb_req+1)));
	ipmb_req->req_seq = seq;
	ipmb_req->responder_lun = 0;
	ipmb_req->command = req->command;
	/* The location of data_checksum field is bogus.
	 * It's used as a placeholder to indicate that a checksum follows the data field.
	 * The location of the data_checksum depends on the size of the data preceeding it.*/
	ipmb_req->data_checksum =
		ipmi_calculate_checksum( &ipmb_req->requester_slave_addr,
			pkt->hdr.req_data_len + 3 );
	req_ws->len_out = sizeof( IPMI_IPMB_REQUEST )
				- IPMB_REQ_MAX_DATA_LEN  +  pkt->hdr.req_data_len;
	/* Assign the checksum to it's proper location */
	*( ( uint8_t * )ipmb_req + req_ws->len_out - 1 ) = ipmb_req->data_checksum;

	//req_ws->ipmi_completion_function = completion_function; //TODO: add completion function
	req_ws->addr_out = dev_addr;
	req_ws->outgoing_protocol = IPMI_CH_PROTOCOL_IPMB;
	req_ws->outgoing_medium = IPMI_CH_MEDIUM_IPMB;
	req_ws->outgoing_channel = ipmi_ch;
	//dump_outgoing( req_ws );

	/* dispatch the request */
    //ws_set_state( req_ws, WS_ACTIVE_MASTER_WRITE );
}
void send_read_fru_data(IPMI_WS *req_ws, uint8_t ipmi_ch, uint8_t dev_addr, unsigned short offset)
{
	IPMI_PKT *pkt;
    printf("send_read_fru_data - ingress \n\r");
	uint8_t seq;

	READ_FRU_DATA_CMD_REQ *req;
	IPMI_IPMB_REQUEST *ipmb_req;

	/*if( !( req_ws = ws_alloc() ) ) {
		return;
	}*/


	pkt = &( req_ws->pkt );

	ipmb_req = ( IPMI_IPMB_REQUEST * )&( req_ws->pkt_out );

	pkt->req = ( IPMI_CMD_REQ * )&( ipmb_req->command ) ;

	req = ( READ_FRU_DATA_CMD_REQ * )pkt->req;

	ipmi_get_next_seq( &seq );

	pkt->hdr.ws = (uint8_t *)req_ws;
	pkt->hdr.req_data_len = sizeof( READ_FRU_DATA_CMD_REQ ) - 1;

	req->command = IPMI_STO_CMD_READ_FRU_DATA;
	req->fru_dev_id = 0;
	req->fru_inventory_offset_lsb = ( uint8_t )offset;
	req->fru_inventory_offset_msb = ( uint8_t )( offset >> 8 );
	req->count_to_read = 20;

	ipmb_req->requester_slave_addr = module_get_i2c_address( I2C_ADDRESS_LOCAL );
	ipmb_req->netfn = NETFN_NVSTORE_REQ;
	ipmb_req->requester_lun = 0;
	ipmb_req->responder_slave_addr = dev_addr;
	ipmb_req->header_checksum = -(ipmb_req->requester_slave_addr + *(( char * )(ipmb_req+1)));
	ipmb_req->req_seq = seq;
	ipmb_req->responder_lun = 0;
	ipmb_req->command = req->command;
	/* The location of data_checksum field is bogus.
	 * It's used as a placeholder to indicate that a checksum follows the data field.
	 * The location of the data_checksum depends on the size of the data preceeding it.*/
	ipmb_req->data_checksum =
		ipmi_calculate_checksum( &ipmb_req->requester_slave_addr,
			pkt->hdr.req_data_len + 3 );
	req_ws->len_out = sizeof( IPMI_IPMB_REQUEST )
				- IPMB_REQ_MAX_DATA_LEN  +  pkt->hdr.req_data_len;

	/* Assign the checksum to it's proper location */
	*( ( uint8_t * )ipmb_req + req_ws->len_out - 1 ) = ipmb_req->data_checksum;

	//req_ws->ipmi_completion_function = completion_function;
	req_ws->addr_out = dev_addr;
	req_ws->outgoing_protocol = IPMI_CH_PROTOCOL_IPMB;
	req_ws->outgoing_medium = IPMI_CH_MEDIUM_IPMB;
	req_ws->outgoing_channel = ipmi_ch;
	//dump_outgoing( req_ws );

	/* dispatch the request */
//	ws_set_state( req_ws, WS_ACTIVE_MASTER_WRITE );
	printf("send_read_fru_data - engress \n\r");
}
#define AMC_EVT_HANDLE_CLOSED_MSG_RCVD		0
#define AMC_EVT_SET_LED_STATE_CMD_OK		1
#define AMC_EVT_READ_CURRENT_REQ_CMD_OK		2
#define AMC_EVT_READ_P2P_RECORD_CMD_OK		3
#define AMC_EVT_ACTIVATION_REQ_MSG_OK		4
#define AMC_EVT_ACTIVATE_FRU_MSG_RCVD		5
#define AMC_EVT_PAYLOAD_ENABLED			6
#define AMC_EVT_HANDLE_OPENED_MSG_RCVD		7
#define AMC_EVT_SET_PORT_STATE_DISABLE_OK	8
#define AMC_EVT_FRU_QUIESCE_CMD_OK		9
#define AMC_EVT_DEVICE_DISCOVERY_OK		10

#define AMC_STATE_M1				0
#define AMC_STATE_M2_LED_LONG_BLINK_SENT	1
#define AMC_STATE_M2_DEVICE_DISCOVERY_STARTED	2
#define AMC_STATE_M2_READ_P2P_RECORD_REQ_SENT	3
#define AMC_STATE_M2_SHM_ACT_REQ_SENT		4
#define AMC_STATE_M2_SHM_ACT_MSG_WAIT		5
#define AMC_STATE_M2_LED_OFF_SENT		6
#define AMC_STATE_M3				7
#define AMC_STATE_M3_PAYLOAD_ENABLE_SENT	8
#define AMC_STATE_M4				9
#define AMC_STATE_M4_LED_BLINK_SENT		10
#define AMC_STATE_M4_PORT_DISABLE_SENT		11
#define AMC_STATE_M4_FRU_QUIESCE_SENT		12

void module_event_handler( IPMI_PKT *pkt )
{
	PLATFORM_EVENT_MESSAGE_CMD_REQ	*req = ( PLATFORM_EVENT_MESSAGE_CMD_REQ * )pkt->req;
	GENERIC_EVENT_MSG *evt_msg = ( GENERIC_EVENT_MSG * )&( req->EvMRev );

	printf( "module_event_handler: ingress\n\r" );
	uint8_t dev_id, dev_addr = (( IPMI_WS * )(pkt->hdr.ws))->incoming_channel;

	dev_id = lookup_dev_id( dev_addr );

	if( evt_msg->sensor_type == IPMI_SENSOR_MODULE_HOT_SWAP ) {

		switch( evt_msg->evt_data1 ) {
			case MODULE_HANDLE_CLOSED:
				printf("MODULE_HANDLE_CLOSED\n\r");
				//mcmc_mmc_event( dev_id, AMC_EVT_HANDLE_CLOSED_MSG_RCVD );

				break;

			case MODULE_HANDLE_OPENED:
				printf("MODULE_HANDLE_OPENED\n\r");
			//	mcmc_mmc_event( dev_id, AMC_EVT_HANDLE_OPENED_MSG_RCVD );
				break;

			case MODULE_QUIESCED:
				printf("MODULE_QUIESCED\n\r");
			//	mcmc_mmc_event( dev_id, AMC_EVT_FRU_QUIESCE_CMD_OK );
				break;

			case MODULE_BACKEND_POWER_FAILURE:
			case MODULE_BACKEND_POWER_SHUTDOWN:
			//	printf("MODULE_BACKEND_POWER_FAILURE OR MODULE_BACKEND_POWER_SHUTDOWN\n\r");
			default:
				printf("module_event_handler not implemented\n\r");
				break;
		}
	} else if ( evt_msg->sensor_type == ST_HOT_SWAP ) {
		FRU_TEMPERATURE_EVENT_MSG_REQ *temp_req = ( FRU_TEMPERATURE_EVENT_MSG_REQ * )req;
		switch( temp_req->evt_reason ) {
			case UPPER_NON_CRITICAL_GOING_HIGH:
				break;
			case UPPER_CRITICAL_GOING_HIGH:
				break;
			case UPPER_NON_RECOVERABLE_GOING_HIGH:
				/* Assume system hardware in jeopardy or damaged */
				/* Turn off all power to AMC cards */
				break;
		}
	}
	else if ( evt_msg->sensor_type == ST_POWER_CHAN_NOT ) {
	PWR_CHANNEL_NOTIFICATION_EVENT_MSG_REQ	*pwr_channel = ( PWR_CHANNEL_NOTIFICATION_EVENT_MSG_REQ * )pkt->req;
			printf("power channel notification\n\r"
					"pwr_channel->event_dir %X\n\r"
					"pwr_channel->event_type %X\n\r"
					"pwr_channel->notification_event_offset %d\n\r"
					"pwr_channel->evt_data2 %d\n\r"
					"pwr_channel->evt_data3 %d\n\r"
					"pwr_channel->pwr_on %d\n\r"
					"pwr_channel->payload_overcurrent %d\n\r"
					"pwr_channel->payload_pwr %d\n\r"
					"pwr_channel->enable %d\n\r"
					"pwr_channel->MG_overcurrent %d\n\r"
					"pwr_channel->mg_pwr %d\n\r"
					"pwr_channel->ps1 %d\n\r"
					"pwr_channel->pwr_channel_num %d\n\r",
			pwr_channel->event_dir,
			pwr_channel->event_type,
			pwr_channel->notification_event_offset,
			pwr_channel->evt_data2,
			pwr_channel->evt_data3,
			pwr_channel->pwr_on,
			pwr_channel->payload_overcurrent,
			pwr_channel->payload_pwr,
			pwr_channel->enable,
			pwr_channel->MG_overcurrent,
			pwr_channel->mg_pwr,
			pwr_channel->ps1,
			pwr_channel->pwr_channel_num);




			}






	printf( "module_event_handler: engress\n\r" );

}

/* mcmc state machine */
void mcmc_mmc_event( uint8_t dev_id, uint8_t event )
{
	uint8_t dev_addr;

	dev_addr = lookup_dev_addr( dev_id );

	switch( event ) {
		case	AMC_EVT_HANDLE_CLOSED_MSG_RCVD:
			/* Send set LED state long blink cmd */
			/* send a “Set FRU LED State” command to the MMC
			 * with a request to perform long blinks of the
			 * BLUE LED, indicating to the operator that the
			 * new Module is waiting to be activated. */
			//send_set_fru_led_state(IPMI_CH_NUM_IPMBL, dev_addr, LED_LONG_BLINK );
			amc[dev_id].state = AMC_STATE_M2_LED_LONG_BLINK_SENT;
			break;
		case	AMC_EVT_SET_LED_STATE_CMD_OK:
			if( amc[dev_id].state == AMC_STATE_M2_LED_LONG_BLINK_SENT ) {
				/* Start device discovery */
				amc[dev_id].state = AMC_STATE_M2_DEVICE_DISCOVERY_STARTED;
			//	device_discovery( dev_id );
			} else if ( amc[dev_id].state == AMC_STATE_M2_LED_OFF_SENT ) {
				amc[dev_id].state = AMC_STATE_M3;
				/* check payload requirements */
				/* enable payload */
				enable_payload( dev_id );
				amc[dev_id].state = AMC_STATE_M3_PAYLOAD_ENABLE_SENT;
			} else if ( amc[dev_id].state == AMC_STATE_M4_LED_BLINK_SENT ) {
				/* send port state disable cmd */
				amc[dev_id].state = AMC_STATE_M4_PORT_DISABLE_SENT;
			}
			break;
		case	AMC_EVT_DEVICE_DISCOVERY_OK:
			// TODO check the fru power record, enable power if within limits
			if( amc[dev_id].state == AMC_STATE_M2_DEVICE_DISCOVERY_STARTED ) {
			//	send_set_fru_led_state( IPMI_CH_NUM_IPMBL, dev_addr, LED_OFF);
				amc[dev_id].state = AMC_STATE_M2_LED_OFF_SENT;
			}
			break;
		case	AMC_EVT_READ_CURRENT_REQ_CMD_OK:
			/* Send read p2p connectivity record cmd */
			amc[dev_id].state = AMC_STATE_M2_READ_P2P_RECORD_REQ_SENT;
			break;
		case	AMC_EVT_READ_P2P_RECORD_CMD_OK:
			/* Send FRU activation req msg to SHM */
			amc[dev_id].state = AMC_STATE_M2_SHM_ACT_REQ_SENT;
			break;
		case	AMC_EVT_ACTIVATION_REQ_MSG_OK:
			amc[dev_id].state = AMC_STATE_M2_SHM_ACT_MSG_WAIT;
			break;
		case	AMC_EVT_ACTIVATE_FRU_MSG_RCVD:
			/* Send set LED state off cmd */
			amc[dev_id].state = AMC_STATE_M2_LED_OFF_SENT;
			break;
		case	AMC_EVT_PAYLOAD_ENABLED:
			amc[dev_id].state = AMC_STATE_M4;
			break;
		case	AMC_EVT_HANDLE_OPENED_MSG_RCVD:
			// Send quiesce req
			//TODO send_fru_control( IPMI_CH_NUM_IPMBL, dev_addr, FRU_CONTROL_QUIESCE, cmd_complete );
			/* Send set LED state short blink cmd */
			//TODO send_set_fru_led_state( IPMI_CH_NUM_IPMBL, dev_addr, LED_SHORT_BLINK, cmd_complete );
			amc[dev_id].state = AMC_STATE_M4_LED_BLINK_SENT;
			break;
		case	AMC_EVT_SET_PORT_STATE_DISABLE_OK:
			/* Send FRU Quiesce cmd */
			amc[dev_id].state = AMC_STATE_M4_FRU_QUIESCE_SENT;
			break;
		case	AMC_EVT_FRU_QUIESCE_CMD_OK:
			amc[dev_id].state = AMC_STATE_M1;
			break;
		default:
			break;
	}
}


void enable_payload( uint8_t dev_id )
{

}

void send_pwr_ctrl_ch( IPMI_WS *req_ws,
		               uint8_t ipmi_ch,
		               uint8_t dev_addr,
		               uint8_t ch_number,
		               uint8_t pwr_ch_crl,
		               uint8_t current_limit,void( *completion_function )( void *, int ))
{
	IPMI_PKT *pkt;
	uint8_t seq;

	PWR_CHANNEL_CTRL_CMD_REQ *req;
	IPMI_IPMB_REQUEST *ipmb_req;

	if( !( req_ws = ws_alloc() ) ) {
		return;
	}

	pkt = &( req_ws->pkt );
	ipmb_req = ( IPMI_IPMB_REQUEST * )&( req_ws->pkt_out );
	pkt->req = ( IPMI_CMD_REQ * )&( ipmb_req->command ) ;
	req = ( PWR_CHANNEL_CTRL_CMD_REQ * )pkt->req;

	ipmi_get_next_seq( &seq );

	pkt->hdr.ws = (uint8_t *)req_ws;
	pkt->hdr.req_data_len = sizeof( PWR_CHANNEL_CTRL_CMD_REQ ) - 1;

	req->command = uTCA_CMD_PWR_CH_CTRL;
	req->picmg_id = 0;
	req->pwr_channel_num=ch_number;

	req->pwr_channel_ctrl=pwr_ch_crl;

	req->pwr_channel_curr_lim = 0xf0;

	req->primary_pm=0x01;
	req->redundant_pm=0x00;

	ipmb_req->requester_slave_addr = module_get_i2c_address( I2C_ADDRESS_LOCAL );
	ipmb_req->netfn = NETFN_PICMG_REQ;
	ipmb_req->requester_lun = 0;
	ipmb_req->responder_slave_addr = dev_addr;
	ipmb_req->header_checksum =-( ipmb_req->responder_slave_addr + *(( char * )(ipmb_req)+1));
	ipmb_req->req_seq = seq;
	ipmb_req->responder_lun = 0;
	ipmb_req->command = req->command;
	/* The location of data_checksum field is bogus.
	 * It's used as a placeholder to indicate that a checksum follows the data field.
	 * The location of the data_checksum depends on the size of the data preceeding it.*/
	ipmb_req->data_checksum =
		ipmi_calculate_checksum( &( ipmb_req->requester_slave_addr ),
			pkt->hdr.req_data_len + 3 );
	req_ws->len_out = sizeof( IPMI_IPMB_REQUEST )
				- IPMB_REQ_MAX_DATA_LEN  +  pkt->hdr.req_data_len;
	/* Assign the checksum to it's proper location */
	*( ( uint8_t * )ipmb_req + req_ws->len_out - 1 ) = ipmb_req->data_checksum;

	req_ws->ipmi_completion_function = completion_function;
	req_ws->addr_out = dev_addr;
	req_ws->outgoing_protocol = IPMI_CH_PROTOCOL_IPMB;
	req_ws->outgoing_medium = IPMI_CH_MEDIUM_IPMB;
	req_ws->outgoing_channel = ipmi_ch;
	//TODO dump_outgoing( req_ws );

	/* dispatch the request */
	ws_set_state(req_ws, WS_ACTIVE_MASTER_WRITE );
}

void get_pwr_channel_stat( IPMI_WS *req_ws, uint8_t ipmi_ch, uint8_t dev_addr, uint8_t start_channel,uint8_t channel_cnt)
{

	IPMI_PKT *pkt;
	uint8_t seq;
	uint8_t responder_slave_addr;
	GET_PWR_CHANNEL_STA_CMD_REQ *req;
	IPMI_IPMB_REQUEST *ipmb_req;

	if( !( req_ws = ws_alloc() ) ) {
	return;
	}

	pkt = &( req_ws->pkt );
	ipmb_req = ( IPMI_IPMB_REQUEST * )&( req_ws->pkt_out );
	pkt->req = ( IPMI_CMD_REQ * )&( ipmb_req->command ) ;
	req = ( GET_PWR_CHANNEL_STA_CMD_REQ * )pkt->req;

	ipmi_get_next_seq( &seq );

	pkt->hdr.ws = (uint8_t *)req_ws;
	pkt->hdr.req_data_len = sizeof( GET_PWR_CHANNEL_STA_CMD_REQ ) - 1;

	req->command = uTCA_CMD_GET_PWR_CH_STAT;
	req->picmg_id = 0;
	req->start_pwr_channel_num=start_channel;
	req->pwr_channel_cnt= channel_cnt;

	ipmb_req->requester_slave_addr = module_get_i2c_address( I2C_ADDRESS_LOCAL );
	ipmb_req->netfn = NETFN_PICMG_REQ;
	ipmb_req->requester_lun = 0;
	responder_slave_addr = dev_addr;
	ipmb_req->header_checksum =-( ipmb_req->responder_slave_addr + *(( char * )(ipmb_req)+1));
	ipmb_req->req_seq = seq;
	ipmb_req->responder_lun = 0;
	ipmb_req->command = req->command;
	/* The location of data_checksum field is bogus.
	 * It's used as a placeholder to indicate that a checksum follows the data field.
	 * The location of the data_checksum depends on the size of the data preceeding it.*/
	ipmb_req->data_checksum =
		ipmi_calculate_checksum( &( ipmb_req->requester_slave_addr ),
			pkt->hdr.req_data_len + 3 );
	req_ws->len_out = sizeof( IPMI_IPMB_REQUEST )
				- IPMB_REQ_MAX_DATA_LEN  +  pkt->hdr.req_data_len;
	/* Assign the checksum to it's proper location */
	*( ( uint8_t * )ipmb_req + req_ws->len_out - 1 ) = ipmb_req->data_checksum;

	//req_ws->ipmi_completion_function = completion_function;
	req_ws->addr_out = dev_addr;
	req_ws->outgoing_protocol = IPMI_CH_PROTOCOL_IPMB;
	req_ws->outgoing_medium = IPMI_CH_MEDIUM_IPMB;
	req_ws->outgoing_channel = ipmi_ch;
	//TODO dump_outgoing( req_ws );

	/* dispatch the request */
	ws_set_state( req_ws, WS_ACTIVE_MASTER_WRITE );
}

void send_fan_ctrl( IPMI_WS *req_ws, uint8_t ipmi_ch, uint8_t dev_addr,uint8_t fru_dev_id,uint8_t fan_level,void( *completion_function )( void *, int ))
{

	    IPMI_PKT *pkt;
		uint8_t seq;

		SET_FAN_LEVEL_CMD_REQ *req;
		IPMI_IPMB_REQUEST *ipmb_req;

		if( !( req_ws = ws_alloc() ) ) {
			return;
		}

		pkt = &( req_ws->pkt );
		ipmb_req = ( IPMI_IPMB_REQUEST * )&( req_ws->pkt_out );
		pkt->req = ( IPMI_CMD_REQ * )&( ipmb_req->command ) ;
		req = ( SET_FAN_LEVEL_CMD_REQ * )pkt->req;

		ipmi_get_next_seq( &seq );

		pkt->hdr.ws = (uint8_t *)req_ws;
		pkt->hdr.req_data_len = sizeof( SET_FAN_LEVEL_CMD_REQ ) - 1;

		req->command = ATCA_CMD_SET_FAN_LEVEL;
		req->picmg_id = 0;
		req->fru_dev_id=fru_dev_id;//TODO: specified power channel to affect
		req->fan_level=fan_level;

		ipmb_req->requester_slave_addr = module_get_i2c_address( I2C_ADDRESS_LOCAL );
		ipmb_req->netfn = NETFN_PICMG_REQ;
		ipmb_req->requester_lun = 0;
		ipmb_req->responder_slave_addr = dev_addr;
		ipmb_req->header_checksum =-( ipmb_req->responder_slave_addr + *(( char * )(ipmb_req)+1));
		printf("header_checkup %02X %02X %02X",ipmb_req->header_checksum,ipmb_req->requester_slave_addr,*(( char * )(ipmb_req)+1));
		ipmb_req->req_seq = seq;
		ipmb_req->responder_lun = 0;
		ipmb_req->command = req->command;
		/* The location of data_checksum field is bogus.
		 * It's used as a placeholder to indicate that a checksum follows the data field.
		 * The location of the data_checksum depends on the size of the data preceeding it.*/
		ipmb_req->data_checksum =
			ipmi_calculate_checksum( &( ipmb_req->requester_slave_addr ),
				pkt->hdr.req_data_len + 3 );
		req_ws->len_out = sizeof( IPMI_IPMB_REQUEST )
					- IPMB_REQ_MAX_DATA_LEN  +  pkt->hdr.req_data_len;
		/* Assign the checksum to it's proper location */
		*( ( uint8_t * )ipmb_req + req_ws->len_out - 1 ) = ipmb_req->data_checksum;

		req_ws->ipmi_completion_function = completion_function;
		req_ws->addr_out = dev_addr;
		req_ws->outgoing_protocol = IPMI_CH_PROTOCOL_IPMB;
		req_ws->outgoing_medium = IPMI_CH_MEDIUM_IPMB;
		req_ws->outgoing_channel = ipmi_ch;
		//TODO dump_outgoing( req_ws );

		/* dispatch the request */
		//TODO ws_set_state( req_ws, WS_ACTIVE_MASTER_WRITE );
}

void send_pm_reset( IPMI_WS *req_ws, uint8_t ipmi_ch, uint8_t dev_addr,uint8_t pm_site_nr)
{

	IPMI_PKT *pkt;
	uint8_t seq;
	uint8_t responder_slave_addr;
	PM_RESET_CMD_REQ *req;
	IPMI_IPMB_REQUEST *ipmb_req;

	/*
		if( !( req_ws = ws_alloc() ) )
	      {
		 return;
		}
		*/
	pkt = &( req_ws->pkt );
	ipmb_req = ( IPMI_IPMB_REQUEST * )&( req_ws->pkt_out );
	pkt->req = ( IPMI_CMD_REQ * )&( ipmb_req->command ) ;
	req = ( PM_RESET_CMD_REQ * )pkt->req;

	ipmi_get_next_seq( &seq );

	pkt->hdr.ws = (uint8_t *)req_ws;
	pkt->hdr.req_data_len = sizeof( PM_RESET_CMD_REQ ) - 1;

	req->command = uTCA_CMD_PM_RST;
	req->picmg_id = 0;
	req->site_nr=pm_site_nr;


	ipmb_req->requester_slave_addr = module_get_i2c_address( I2C_ADDRESS_LOCAL );
	ipmb_req->netfn = NETFN_PICMG_REQ;
	ipmb_req->requester_lun = 0;
	responder_slave_addr = dev_addr;
	ipmb_req->header_checksum =-( ipmb_req->responder_slave_addr + *(( char * )(ipmb_req)+1));
	ipmb_req->req_seq = seq;
	ipmb_req->responder_lun = 0;
	ipmb_req->command = req->command;
	/* The location of data_checksum field is bogus.
	 * It's used as a placeholder to indicate that a checksum follows the data field.
	 * The location of the data_checksum depends on the size of the data preceeding it.*/
	ipmb_req->data_checksum =
		ipmi_calculate_checksum( &( ipmb_req->requester_slave_addr ),
			pkt->hdr.req_data_len + 3 );
	req_ws->len_out = sizeof( IPMI_IPMB_REQUEST )
				- IPMB_REQ_MAX_DATA_LEN  +  pkt->hdr.req_data_len;
	/* Assign the checksum to it's proper location */
	*( ( uint8_t * )ipmb_req + req_ws->len_out - 1 ) = ipmb_req->data_checksum;

	//req_ws->ipmi_completion_function = completion_function;
	req_ws->addr_out = dev_addr;
	req_ws->outgoing_protocol = IPMI_CH_PROTOCOL_IPMB;
	req_ws->outgoing_medium = IPMI_CH_MEDIUM_IPMB;
	req_ws->outgoing_channel = ipmi_ch;
	//TODO dump_outgoing( req_ws );

	/* dispatch the request */
	//TODO ws_set_state( req_ws, WS_ACTIVE_MASTER_WRITE );

}

void send_get_pm_status(IPMI_WS *req_ws, uint8_t ipmi_ch, uint8_t dev_addr,uint8_t pm_site_nr)
{
		IPMI_PKT *pkt;
		uint8_t seq;
		uint8_t responder_slave_addr;
		GET_PM_STAT_CMD_REQ *req;
		IPMI_IPMB_REQUEST *ipmb_req;


		if( !( req_ws = ws_alloc() ) )
		{
			printfr(PSTR("no available slot for the command. exit\n\r"));
			return;
		}

		pkt = &( req_ws->pkt );
		ipmb_req = ( IPMI_IPMB_REQUEST * )&( req_ws->pkt_out );
		pkt->req = ( IPMI_CMD_REQ * )&( ipmb_req->command ) ;
		req = ( GET_PM_STAT_CMD_REQ * )pkt->req;

		ipmi_get_next_seq( &seq );

		pkt->hdr.ws = (uint8_t *)req_ws;
		pkt->hdr.req_data_len = sizeof( GET_PM_STAT_CMD_REQ ) - 1;

		req->command = uTCA_CMD_GET_PM_STAT;
		req->picmg_id = 0;
		req->site_nr=pm_site_nr;


		ipmb_req->requester_slave_addr = module_get_i2c_address( I2C_ADDRESS_LOCAL );
		ipmb_req->netfn = NETFN_PICMG_REQ;
		ipmb_req->requester_lun = 0;
		responder_slave_addr = dev_addr;
		ipmb_req->header_checksum =-( ipmb_req->responder_slave_addr + *(( char * )(ipmb_req)+1));
		ipmb_req->req_seq = seq;
		ipmb_req->responder_lun = 0;
		ipmb_req->command = req->command;
		/* The location of data_checksum field is bogus.
		 * It's used as a placeholder to indicate that a checksum follows the data field.
		 * The location of the data_checksum depends on the size of the data preceeding it.*/
		ipmb_req->data_checksum =
			ipmi_calculate_checksum( &( ipmb_req->requester_slave_addr ),
				pkt->hdr.req_data_len + 3 );
		req_ws->len_out = sizeof( IPMI_IPMB_REQUEST )
					- IPMB_REQ_MAX_DATA_LEN  +  pkt->hdr.req_data_len;
		/* Assign the checksum to it's proper location */
		*( ( uint8_t * )ipmb_req + req_ws->len_out - 1 ) = ipmb_req->data_checksum;

		//req_ws->ipmi_completion_function = completion_function;
		req_ws->addr_out = dev_addr;
		req_ws->outgoing_protocol = IPMI_CH_PROTOCOL_IPMB;
		req_ws->outgoing_medium = IPMI_CH_MEDIUM_IPMB;
		req_ws->outgoing_channel = ipmi_ch;
		//TODO dump_outgoing( req_ws );

		/* dispatch the request */
		ws_set_state( req_ws, WS_ACTIVE_MASTER_WRITE );
}
/*==============================================================
 * INITIALIZATION
 *==============================================================*/
/*
 * module_init()
 *
 * Gets called when we power up.
 */

void
module_init( void )
{
	module_init2();
	module_sensor_init();

}


void
module_init2( void )
{

	// get our IPMB-L address
	mcmc_ipmbl_address = module_get_i2c_address( I2C_ADDRESS_LOCAL );

	// module specific initialization for serial & i2c interfaces go in here

}

void
module_sensor_init( void )
{
	unsigned char dev_slave_addr =  module_get_i2c_address( I2C_ADDRESS_LOCAL );;
	/*
	sdr1.dev_slave_addr = dev_slave_addr;
	sdr_entry_table[0].record_ptr = (unsigned char *)&sdr1;
	sdr_entry_table[0].rec_len = 25;
	sdr_entry_table[0].record_id = current_sensor_count;
	current_sensor_count = 1;

	sdr2.owner_id = dev_slave_addr;
	sdr_entry_table[1].record_ptr = (unsigned char *)&sdr2;
	sdr_entry_table[1].rec_len = 42;
	sdr_entry_table[1].record_id = current_sensor_count;
	current_sensor_count = 2;

	sdr3.owner_id = dev_slave_addr;
	sdr_entry_table[2].record_ptr = (unsigned char *)&sdr3;
	sdr_entry_table[2].rec_len = 53;
	sdr_entry_table[2].record_id = current_sensor_count;
	current_sensor_count = 3;

	sdr3.owner_id = dev_slave_addr;
	sdr_entry_table[3].record_ptr = (unsigned char *)&sdr4;
	sdr_entry_table[3].rec_len = 53;
	sdr_entry_table[3].record_id = current_sensor_count;
	current_sensor_count = 4;

		// TODO fix i2c_addr in tm100_init()
	*
	*/
	tm100_init_sensor_record();
	//uint8_t TMP_100_ADDR[] ={0x90,0x98, 0x9C, 0x94};
	tm100_init(1, 0x90);
	tm100_init(1, 0x94);
}


void
module_process_response(
	IPMI_WS *req_ws,
	unsigned char seq,
	unsigned char completion_code )
{
	switch( completion_code ) {
		case CC_NORMAL:
			/*
			 * Life is perfect!
			 *
			 */
			printfr(PSTR("Response package OK\n\r"));

			break;
		case CC_BUSY:
		case CC_INVALID_CMD:
		case CC_INVALID_CMD_FOR_LUN:
		case CC_TIMEOUT:
		case CC_OUT_OF_SPACE:
		case CC_RESERVATION:
		case CC_RQST_DATA_TRUNCATED:
		case CC_RQST_DATA_LEN_INVALID:
		case CC_DATA_LEN_EXCEEDED:
		case CC_PARAM_OUT_OF_RANGE:
		case CC_CANT_RETURN_REQ_BYTES:
		case CC_REQ_DATA_NOT_AVAIL:
		case CC_INVALID_DATA_IN_REQ:
		case CC_CMD_ILLEGAL:
		case CC_CMD_RESP_NOT_PROVIDED:
		case CC_CANT_EXECUTE_DUP_REQ:
		case CC_SDR_IN_UPDATE_MODE:
		case CC_FW_IN_UPDATE_MODE:
		case CC_INITIALIZATION:
		case CC_DEST_UNAVAILABLE:
		case CC_SECURITY_RESTRICTION:
		case CC_NOT_SUPPORTED:
		case CC_PARAM_ILLEGAL:
		case CC_UNSPECIFIED_ERROR:
			/*
			 * Myterious problem happen check this out
			 */
			printfr(PSTR("Response package NOK\n\r"));
			break;
		default:
			break;
	}

}

