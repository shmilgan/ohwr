#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <inttypes.h>
#include <sys/time.h>

#include "ptpd_netif.h"
#include "hal_exports.h"
#include <hw/fpga_regs.h>
#include <hw/pps_gen_regs.h>

#define PPS_GEN_BASE 0x1c0000

#define PTYPE_ANNOUNCE 1
#define PTYPE_ANNOUNCE_RESP 2
#define PTYPE_DELAY_REQ 3
#define PTYPE_DELAY_RESP 4
#define PTYPE_DELAY_REQ_FOLLOWUP 5
#define PTYPE_DELAY_RESP_FOLLOWUP 6
#define PTYPE_CALIBRATION_REQUEST 7
#define PTYPE_CALIBRATION_ACK 8
#define PTYPE_LOCK_REQUEST 9
#define PTYPE_LOCK_ACK 10

#define PATTERN_ON 2
#define GET_DELTAS 3

#define CAL_ERROR -1

typedef struct {
  uint64_t utc;
  uint32_t nsec;
  uint32_t phase;
} timestamp_t;

typedef struct {
  int ptype;
  int seq;
  struct  {
    wr_timestamp_t t1, t2, t3, t4;
  } delay;
	
  struct  {
    int cmd;
    int ack;
    int pattern_duration;
    uint64_t delta_tx;
    uint64_t delta_rx;
    int drx_valid, dtx_valid;
  } cal;
	
	
  struct {
    int is_master;
  } announce;

  struct {
    int ok;
  } announce_resp;
} sync_packet_t;

#define ST_INIT 0
#define ST_WAIT_LINK 1
#define ST_S_WAIT_ANNOUNCE 2
#define ST_M_SEND_ANNOUNCE 3
#define ST_M_CALIBRATE 4
#define ST_S_CALIBRATE 5
#define ST_S_WAIT_COMMAND 6
#define ST_M_WAIT_SLAVE_LOCK 7
#define ST_M_WAIT_CALIBRATE 8
#define ST_S_WAIT_LOCK 9
#define ST_M_SEND_SYNC 10
#define ST_M_SEND_SYNC_FOLLOWUP 11
#define ST_S_WAIT_FOLLOWUP 12

typedef struct
{
  uint64_t start_tics;
  uint64_t timeout;
} timeout_t ;

uint64_t get_tics()
{
  struct timezone tz = {0,0};
  struct timeval tv;
  gettimeofday(&tv, &tz);
	
  return (uint64_t) tv.tv_sec * 1000000ULL + tv.tv_usec;
}


static inline int tmo_init(timeout_t *tmo, uint32_t milliseconds)
{
  tmo->start_tics = get_tics();
  tmo->timeout = (uint64_t) milliseconds * 1000ULL;
}

static inline int tmo_restart(timeout_t *tmo)
{
  tmo->start_tics = get_tics();
}

static inline int tmo_expired(timeout_t *tmo)
{
  return (get_tics() - tmo->start_tics > tmo->timeout);
}



#define ANNOUNCE_INTERVAL 200
#define LINK_POLL_INTERVAL 200

#define CALIBRATION_TIME 1000

#define LOCK_CHECK_INTERVAL 500

#define MASTER_PORT "wru1"
#define SLAVE_PORT "wru1"

#define OUR_ETHERTYPE 0x88f8
#define OUR_MCAST_ADDR { 0x01, 0x80, 0xc2, 0x01, 0x00, 0x00 }

int check_link_up(char *iface)
{
  hexp_port_state_t state;
  int rval = halexp_get_port_state(&state, iface);
  return state.up;
}

wr_timestamp_t tms_sub(wr_timestamp_t a, wr_timestamp_t b)
{
  wr_timestamp_t c;
  if(a.nsec < b.nsec)
    {
      c.nsec = 1000000000 + a.nsec - b.nsec;
      c.utc = -1;
    } else {
    c.nsec = a.nsec - b.nsec;
    c.utc = 0;
  }

  c.utc += a.utc - b.utc;
  return c;
}

void sync_pkt_send(wr_socket_t *sock, sync_packet_t *pkt, wr_timestamp_t *tx_ts)
{
  uint8_t mac[] = OUR_MCAST_ADDR;
  wr_sockaddr_t send_addr;
 
  send_addr.ethertype = OUR_ETHERTYPE;
  memcpy(send_addr.mac, mac, 6); 

  ptpd_netif_sendto(sock, &send_addr, pkt, sizeof(sync_packet_t), tx_ts);
}


int sync_pkt_receive(wr_socket_t *sock, sync_packet_t *pkt, wr_timestamp_t *ts)
{
  wr_sockaddr_t from;
  int nrx = ptpd_netif_recvfrom(sock, &from, pkt, sizeof(sync_packet_t), ts);
	
  return (nrx == sizeof(sync_packet_t) && from.ethertype == OUR_ETHERTYPE);
}

static inline int inside_range(int min, int max, int x)
{
  if(min < max) 
    return (x>=min && x<=max);
  else
    return (x<=max || x>=min);
}



#define TS_T2 1
#define TS_T4 2

void linearize_timestamp(int ts_type, wr_timestamp_t *ts, uint32_t dmtd_phase, hexp_port_state_t *pstate)
{
  int trip_lo, trip_hi;


  // "phase" transition: DMTD output value (in picoseconds) at which the transition of rising edge
  // TS counter will appear

  uint32_t phase_trans;

  phase_trans = (ts_type == TS_T2 ? pstate->t2_phase_transition : pstate->t4_phase_transition);

  // calculate the range within which falling edge timestamp is stable (no possible transitions)
  trip_lo = phase_trans - pstate->clock_period / 4;
  if(trip_lo < 0) trip_lo += pstate->clock_period;

  trip_hi = phase_trans + pstate->clock_period / 4;
  if(trip_hi >= pstate->clock_period) trip_hi -= pstate->clock_period;

  //  fprintf(stderr,"phase %d dtrans %d period %d\n", dmtd_phase, phase_trans, pstate->clock_period);

  if(inside_range(trip_lo, trip_hi, dmtd_phase))
    {
      // We are within +- 25% range of transition area of rising counter. Take the falling edge counter value
      // as the "reliable" one.

      ts->nsec -= ts->cntr_ahead ? (pstate->clock_period / 1000) : 0;

      // check if the phase is before the counter transition value and eventually increase the counter by 1
      // to simulate a timestamp transition exactly at phase_trans DMTD phase value
      if(inside_range(trip_lo, phase_trans, dmtd_phase))
	ts->nsec += pstate->clock_period / 1000;

    }

  ts->phase = dmtd_phase - phase_trans - 1;
  if(ts->phase  < 0) ts->phase += pstate->clock_period;
  ts->phase = pstate->clock_period -1 -ts->phase;
}	   




void master_fsm(char *if_name)
{
  int state = ST_INIT;
  timeout_t tmo;
  wr_socket_t *m_sock;
  wr_sockaddr_t sock_addr;
  sync_packet_t tx_pkt, rx_pkt;
  hexp_port_state_t port_state;

  wr_frame_tag_t sync_tag;
  wr_timestamp_t sync_t1, sync_t2, sync_t3, sync_t4, rx_ts;

  int got_packet;
  int seq = 0;
	
  strcpy(sock_addr.if_name, if_name);
  sock_addr.family = PTPD_SOCK_RAW_ETHERNET; // socket type
  sock_addr.ethertype = OUR_ETHERTYPE;
  memset(sock_addr.mac, 0, 6); 

  m_sock = ptpd_netif_create_socket(PTPD_SOCK_RAW_ETHERNET, 0, &sock_addr);
	
	
  fprintf(stderr,"Running as a master\n");


  FILE *f_phlog = fopen("/tmp/phase_log_master", "wb");
	
	
  for(;;)
    {


      got_packet = sync_pkt_receive(m_sock, &rx_pkt, &rx_ts);

      switch(state)
	{
	case ST_INIT:
	  tmo_init(&tmo, LINK_POLL_INTERVAL);
	  state = ST_WAIT_LINK;
	  fprintf(stderr, "[master] Waiting for the link to go up");
		  
	  break;
		
	case ST_WAIT_LINK:
	  if(tmo_expired(&tmo))
	    {
	      if(check_link_up(if_name))
		{
		  fprintf(stderr,"\n[master] Link up.\n");
		  state = ST_M_SEND_ANNOUNCE;
		  tmo_init(&tmo, ANNOUNCE_INTERVAL);
		} else {
		fprintf(stderr, ".");
		tmo_restart(&tmo);
	      }
	    }
	  break;
			
	case ST_M_SEND_ANNOUNCE:
	  if(tmo_expired(&tmo))
	    {
	      tx_pkt.ptype = PTYPE_ANNOUNCE;
	      tx_pkt.announce.is_master = 1;
			  
	      sync_pkt_send(m_sock,  &tx_pkt, NULL);

	      tmo_restart(&tmo);
	    }
			
		
			
	  //check if we've got announce response from the slave
	  if(got_packet && rx_pkt.ptype == PTYPE_ANNOUNCE_RESP)
	    {
	      tx_pkt.ptype = PTYPE_LOCK_REQUEST;
	      sync_pkt_send(m_sock, &tx_pkt, NULL);
	      state = ST_M_WAIT_SLAVE_LOCK;
	      fprintf(stderr,"[master] Got ANNOUNCE_RESP, sending LOCK command.\n");
	    }
				
	  break;
		
	case ST_M_WAIT_SLAVE_LOCK:
	  {
	    if(got_packet  && rx_pkt.ptype == PTYPE_LOCK_ACK)
	      {
				 
		//		state = ST_M_WAIT_CALIBRATE;
		state = ST_M_SEND_SYNC;
		fprintf(stderr,"[master] Slave ACKed its lock.\n");
		tmo_init(&tmo, 10);

	      }
	    break;
	  }
		 
		
	case ST_M_WAIT_CALIBRATE:
	  {
		 
	  }
	  break;
		
	case ST_M_SEND_SYNC:

	  if(tmo_expired(&tmo))
	    {
	      tx_pkt.ptype = PTYPE_DELAY_REQ;
	      tx_pkt.seq = seq;
	      sync_pkt_send(m_sock, &tx_pkt, &sync_t1);
	      tx_pkt.ptype = PTYPE_DELAY_REQ_FOLLOWUP;
	      tx_pkt.delay.t1 = sync_t1;
	      tx_pkt.seq = seq;
	      sync_pkt_send(m_sock, &tx_pkt, NULL);
	      tmo_restart(&tmo);
	      seq++;
	    }

	  if(got_packet && rx_pkt.ptype == PTYPE_DELAY_RESP)
	    {
	      fprintf(stderr,"[master] Send Got DELAY_RESP.\n");
	      
	      sync_t4 =rx_ts;
	      halexp_get_port_state(&port_state, if_name);
	      linearize_timestamp(TS_T4, &sync_t4, port_state.phase_val, &port_state);
	    } else	  if(got_packet && rx_pkt.ptype == PTYPE_DELAY_RESP_FOLLOWUP)
	    {
	      sync_t3 = rx_pkt.delay.t3;
	      wr_timestamp_t dt = tms_sub(sync_t4, sync_t3);
	      
	      fprintf(stderr, "%d %lld %d %d\n", port_state.phase_val, dt.utc, dt.nsec, sync_t4.phase);      
	      fprintf(f_phlog, "%d %lld %d %d\n", port_state.phase_val, dt.utc, dt.nsec, sync_t4.phase);


	    }


	  break;

		
	}
      usleep(1000);
    }
}

/*
#define TS_T2 1
#define TS_T4 2

void linearize_timestamp(wr_timestamp_t *ts, hexp_port_state_t *pstate)
{
  int trip_lo, trip_hi;

  uint32_t ts_fedge = t2->cntr_ahead * 8 + t2->nsec;

  trip_lo = PHASE_TRIP_UP1 - PHASE_MAX/4;
  if(trip_lo < 0) trip_lo += PHASE_MAX;

  trip_hi = PHASE_TRIP_UP1 + PHASE_MAX/4;
  if(trip_hi >= PHASE_MAX) trip_hi -= PHASE_MAX;

  if(inside_range(trip_lo, trip_hi, dmtd_val))
    {
      t2->nsec -= t2->cntr_ahead ? REFCLK_PERIOD : 0;

      if(inside_range(trip_lo, PHASE_TRIP_UP1, dmtd_val))
	t2->nsec += REFCLK_PERIOD;

      //      fprintf(stderr,"UseFalling %d", t2->cntr_ahead);
    }

  t2->phase = dmtd_val - PHASE_TRIP_UP1 - 1;
  if(t2->phase  < 0) t2->phase += PHASE_MAX;
  t2->phase = PHASE_MAX-1-t2->phase;
}	   
*/


void slave_fsm(char *if_name)
{
  int state = ST_INIT;
  timeout_t tmo;
  wr_socket_t *m_sock;
  wr_sockaddr_t sock_addr;
  sync_packet_t tx_pkt, rx_pkt;
  int got_packet;
  hexp_port_state_t port_state;
  wr_timestamp_t rx_ts, t1, t2, t3;
  int phase = 0;

  FILE *f_phlog = fopen("/tmp/phase_log_slave", "wb");
	
  strcpy(sock_addr.if_name, if_name);
  sock_addr.family = PTPD_SOCK_RAW_ETHERNET; // socket type
  sock_addr.ethertype = OUR_ETHERTYPE;
  memset(sock_addr.mac, 0, 6); 

  m_sock = ptpd_netif_create_socket(PTPD_SOCK_RAW_ETHERNET, 0, &sock_addr);
	
	
  fprintf(stderr,"Running as a slave\n");

  for(;;)
    {

      got_packet = sync_pkt_receive(m_sock, &rx_pkt, &rx_ts);

      switch(state)
	{
	case ST_INIT:
	  tmo_init(&tmo, LINK_POLL_INTERVAL);
	  state = ST_WAIT_LINK;
	  fprintf(stderr, "Waiting for the link to go up");
		  
	  break;
		
	case ST_WAIT_LINK:
	  if(tmo_expired(&tmo))
	    {
	      if(check_link_up(if_name))
		{
		  fprintf(stderr,"\n[slave] Link up.\n");
		  state = ST_S_WAIT_ANNOUNCE;
		  tmo_init(&tmo, ANNOUNCE_INTERVAL);
		} else {
		fprintf(stderr, ".");
		tmo_restart(&tmo);
	      }
	    }
	  break;
			
	case ST_S_WAIT_ANNOUNCE: 
	  if(got_packet && rx_pkt.ptype == PTYPE_ANNOUNCE)
	    {
	      tx_pkt.ptype = PTYPE_ANNOUNCE_RESP;
	      tx_pkt.announce_resp.ok = 1;
	      fprintf(stderr,"[slave] Got ANNOUNCE message!\n");
	      sync_pkt_send(m_sock, &tx_pkt, NULL);
	      halexp_lock_cmd(SLAVE_PORT, HEXP_LOCK_CMD_START, 0);
	      state = ST_S_WAIT_LOCK;
	      tmo_init(&tmo, LOCK_CHECK_INTERVAL);
	    }
	  break;
				
	case ST_S_WAIT_LOCK:
	  {
	    if(tmo_expired(&tmo))
	      {
		tmo_restart(&tmo);
		int rval = halexp_lock_cmd(SLAVE_PORT, HEXP_LOCK_CMD_CHECK, 0);
 						
 						
		if(rval  == HEXP_LOCK_STATUS_LOCKED)
		  {

		    
		    

		    fprintf(stderr, "[slave] Port %s locked.\n", if_name);
		    tx_pkt.ptype = PTYPE_LOCK_ACK;
		    sync_pkt_send(m_sock, &tx_pkt, NULL);
		    state = ST_S_WAIT_COMMAND;

		    tmo_init(&tmo, 500);

		  }
	      }
	    break;
	  }
	
	case ST_S_WAIT_COMMAND:
	  if(got_packet && rx_pkt.ptype == PTYPE_DELAY_REQ)
	    {
	      state = ST_S_WAIT_FOLLOWUP;
//	      fprintf(stderr, "[slave] Got DELAY_REQ [seq %d].\n", rx_pkt.seq);
	      t2 = rx_ts;
	    }	  else  if(tmo_expired(&tmo))
	    {
	      tmo_restart(&tmo);

	//      fprintf(stderr,"[slave] PhaseVal: %d valid: %d\n", (uint32_t) port_state.phase_val, port_state.phase_val_valid);
	    }
	  break;

	case ST_S_WAIT_FOLLOWUP:
	  if(got_packet && rx_pkt.ptype == PTYPE_DELAY_REQ_FOLLOWUP)
	    {
	      t1 = rx_pkt.delay.t1;
	      //    fprintf(stderr, "[slave] Got DELAY_REQ_FOLLOWUP [seq %d].\n", rx_pkt.seq);
	      //	      fprintf(stderr, "[slave] t1r=%lld.%d t2r=%lld.%d\n", t1.utc, t1.nsec,t2.utc, t2.nsec);


	      state = ST_S_WAIT_COMMAND;

	      tx_pkt.ptype = PTYPE_DELAY_RESP;
	      sync_pkt_send(m_sock, &tx_pkt, &t3);

	      tx_pkt.ptype = PTYPE_DELAY_RESP_FOLLOWUP;
	      tx_pkt.delay.t3 = t3;
	      sync_pkt_send(m_sock, &tx_pkt, NULL);


	      halexp_get_port_state(&port_state, if_name);
	      linearize_timestamp(TS_T2, &t2, port_state.phase_val, &port_state);
	      
	      t1.phase = 0;

	      wr_timestamp_t dt = tms_sub(t2,t1);

	      fprintf(stderr, "%d %lld %d %d\n", phase, dt.utc, dt.nsec, t2.phase);
	      fprintf(f_phlog, "%d %lld %d %d\n", phase, dt.utc, dt.nsec, t2.phase);
		      
	      fflush(f_phlog);
// increase the phase
	      hexp_pps_params_t pps_params;
	      pps_params.adjust_phase_shift = phase;
	      phase+=10;
	      halexp_pps_cmd(HEXP_PPSG_CMD_ADJUST_PHASE, &pps_params);


	    }
	  break;
	  

	}
    }

}


main(int argc, char *argv[])
{

  if(argc != 3)
    {
      fprintf(stderr,"Usage: %s [-m/-s] iface\n\n",argv[0]);
      return 0;
    }

  if(halexp_client_init() < 0)
    {
      fprintf(stderr,"Unable to connect to the WR HAL daemon. Sorry....\n");
      return -1;
    }
	

  ptpd_netif_init();
  
  if(!strcmp(argv[1], "-m"))
    master_fsm(argv[2]);
  else 	if(!strcmp(argv[1], "-s"))
    slave_fsm(argv[2]);
  else{
    fprintf(stderr,"Invalid parameter: %s\n", argv[1]);
    return -1;
  }
  return 0;
}
