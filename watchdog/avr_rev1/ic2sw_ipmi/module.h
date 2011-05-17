/*
 * module.h
 *
 *  Created on: 24-Jun-2009
 *      Author: poliveir
 */

#ifndef MODULE_H_
#define MODULE_H_


void module_init( void );
void module_sensor_init( void );
void module_event_handler( IPMI_PKT *pkt );
unsigned char module_get_i2c_address( int address_type );
void send_read_fru_data(IPMI_WS *req_ws, uint8_t ipmi_ch, uint8_t dev_addr, unsigned short offset);
void send_pwr_ctrl_ch( IPMI_WS *req_ws, uint8_t ipmi_ch, uint8_t dev_addr, uint8_t ch_number,uint8_t pwr_ch_crl,uint8_t current_limit,void( *completion_function )( void *, int ));
void send_fan_ctrl( IPMI_WS *req_ws, uint8_t ipmi_ch, uint8_t dev_addr,uint8_t fru_dev_id,uint8_t fan_level,void( *completion_function )( void *, int ));
void send_picmg_properties(IPMI_WS *req_ws, uint8_t ipmi_ch, uint8_t dev_addr);
void get_pwr_channel_stat( IPMI_WS *req_ws, uint8_t ipmi_ch, uint8_t dev_addr, uint8_t start_channel,uint8_t channel_cnt);
void send_get_pm_status(IPMI_WS *req_ws, uint8_t ipmi_ch, uint8_t dev_addr,uint8_t pm_site_nr);
void module_process_response( IPMI_WS *req_ws,unsigned char seq,unsigned char completion_code);
void cmd_complete( IPMI_WS *ws, int status );

#endif /* MODULE_H_ */
