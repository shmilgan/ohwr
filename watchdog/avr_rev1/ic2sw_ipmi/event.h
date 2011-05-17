/*

*/
#ifndef EVENT_H
#define EVENT_H

void ipmi_get_pef_capabilities( IPMI_PKT *pkt );
void ipmi_arm_pef_postpone_timer( IPMI_PKT *pkt );
void ipmi_set_pef_config_params( IPMI_PKT *pkt );
void ipmi_get_pef_config_params( IPMI_PKT *pkt );
void ipmi_set_last_processed_event( IPMI_PKT *pkt );
void ipmi_get_last_processed_event( IPMI_PKT *pkt );
void ipmi_platform_event( IPMI_PKT *pkt );
void ipmi_set_event_receiver( IPMI_PKT *pkt );
void ipmi_get_event_receiver( IPMI_PKT *pkt );
int event_data_compare( uint8_t test_value, PEF_MASK *pef_mask );
int ipmi_send_event_req( uint8_t *msg_cmd, uint8_t msg_len, void(*ipmi_completion_function)( void *, int ) );
void ipmi_process_event_req( IPMI_PKT *pkt );

#endif
