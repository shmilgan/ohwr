/*\\
 * White Rabbit RTU (Routing Table Unit)
 * Copyright (C) 2010, CERN.
 *
 * Version:     wrsw_rtud v1.0
 *
 * Authors:     Juan Luis Manas (juan.manas@integrasys.es)
 *              Miguel Baizan   (miguel.baizan@integrasys.es)
 *
 * Description: RTU daemon. 
 *              Handles the learning and aging processes. 
 *              Manages the filtering and VLAN databases.
 *
 * Fixes:       
 *              Alessandro Rubini
 *              Tomasz Wlostowski 
 *              
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>


#include <trace.h>
#include <switch_hw.h>
#include <hal_client.h>

#include "rtu.h"
#include "mac.h"
#include "rtu_fd.h"
#include "rtu_drv.h"
#include "rtu_ext_drv.h"
#include "rtu_hash.h"
#include "utils.h"


static pthread_t aging_process;
static pthread_t wripc_process;
static pthread_t truud_process; //tru update process

static struct {
	int in_use;
	int is_up;
	char if_name[16];
	int hw_index;
} port_state[MAX_PORT + 1];


/**
 * \brief Creates the static entries in the filtering database 
 * @return error code
 */

static int rtu_create_static_entries()
{
    uint8_t bcast_mac[]         = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    uint8_t slow_proto_mac[]    = {0x01, 0x80, 0xc2, 0x00, 0x00, 0x01};
    uint8_t ptp_mcast_mac[]     = {0x01, 0x1b, 0x19, 0x00, 0x00, 0x00};
    hexp_port_state_t pstate;
		hexp_port_list_t ports;
    int i, err;
    uint32_t enabled_port_mask = 0;


		halexp_query_ports(&ports);
		
		TRACE(TRACE_INFO, "Number of physical ports: %d, active ports: %d\n", ports.num_physical_ports, ports.num_ports);
	
    // VLAN-aware Bridge reserved addresses (802.1Q-2005 Table 8.1)
    TRACE(TRACE_INFO,"adding static routes for slow protocols...");
    for(i = 0; i < NUM_RESERVED_ADDR; i++) {
        slow_proto_mac[5] = i;
        err = rtu_fd_create_entry(slow_proto_mac, 0, (1 << ports.num_physical_ports), STATIC);
        if(err)
            return err;
    }
    
  	memset(port_state, 0, sizeof(port_state));
  	
    for(i = 0; i < ports.num_ports; i++) {
        halexp_get_port_state(&pstate, ports.port_names[i]); 
        enabled_port_mask |= (1 << pstate.hw_index);
        
        strncpy(port_state[i].if_name, ports.port_names[i], 16);
        port_state[i].is_up = 0;
        port_state[i].hw_index = pstate.hw_index;
    		port_state[i].in_use = 1;
    		    
        TRACE(
            TRACE_INFO,
            "adding static route for port %s index %d [mac %s]", 
            ports.port_names[i], 
            pstate.hw_index, 
            mac_to_string(pstate.hw_addr)
        );

				err = rtu_fd_create_entry(pstate.hw_addr, 0, (1 << ports.num_physical_ports), STATIC);
        if(err)
            return err;
    }

    // Broadcast MAC
    TRACE(TRACE_INFO,"adding static route for broadcast MAC...");
    err = rtu_fd_create_entry(bcast_mac, 0, enabled_port_mask | (1 << ports.num_physical_ports), STATIC);
    err = rtu_fd_create_entry(ptp_mcast_mac, 0, (1 << ports.num_physical_ports), STATIC);
    if(err)
        return err;

    TRACE(TRACE_INFO,"done creating static entries.");

    return 0;
}


/* Checks if the link is up on inteface (if_name). Returns non-zero if yes. */
static int check_link(int fd_raw, const char *if_name)
{
  struct ifreq ifr;

  strncpy(ifr.ifr_name, if_name, sizeof(ifr.ifr_name));
  if(ioctl(fd_raw, SIOCGIFFLAGS, &ifr) > 0) return -1;
  return (ifr.ifr_flags & IFF_UP && ifr.ifr_flags & IFF_RUNNING);
}

static void rtu_update_ports_state()
{
	int i;
	static int fd_raw = -1;

	/* create a dummy socket for ioctls() to check link status on each port */
	if(fd_raw < 0)
	{
		fd_raw = socket(AF_PACKET, SOCK_DGRAM, 0);
	  if(fd_raw < 0) 
	  	return;
	}
	
	for(i=0; i <= MAX_PORT; i++)
	{
		if(!port_state[i].in_use)
			continue;
			
		int link_up = check_link(fd_raw, port_state[i].if_name);
		if(port_state[i].is_up && !link_up)
		{
			TRACE(TRACE_INFO, "Port %s went down, removing corresponding entries...", port_state[i].if_name)
			
			rtu_fd_clear_entries_for_port(port_state[i].hw_index);
		} 
		
		port_state[i].is_up = link_up;
		
	}
}
#define TRU_PORT_MASK    0x0E
#define TRU_ALL_WORKS    0x06
#define TRU_PORT_1       0x02
#define TRU_PORT_2       0x04
#define INITIAL_ACTIVE_PORT 1
#define INITIAL_BACKUP_PORT 2



/*
 input_active_port:
                    >  0 - indictest active port to be set (run from command line, not deamon)
                    = -1 - indicates that automatic update of active port should be done
                    = -2 - indicates that update should be made with no change of active port
 */
static void tru_update_ports_state(int input_active_port )
{
   int static active_port = INITIAL_ACTIVE_PORT;
   int static backup_port = INITIAL_BACKUP_PORT;
   int change_active_port = 0;
   int i;
   int port_state, port_setting;
   int remembered_active_port;
   uint32_t ports_stable_up, ports_up, bank, port_down, port_woke_up, port_up, ports_setting;
   if(input_active_port >= 0) // run not from daemon
   {
      int err = shw_fpga_mmap_init();
      if(err)
      {
         TRACE(TRACE_INFO, "Problem with fpga mapping");
         exit(1);
      }
   }
   remembered_active_port =  active_port;
   // disable ports which are down
   ports_setting = 0;
   tru_read_status(&bank, &ports_up, &ports_stable_up,0);
   for(i=0; i <= MAX_PORT; i++)
   {
       port_state   = 0x1 & (ports_up >> i);
       port_setting = rtu_port_setting(i);
       if (port_setting == 1 &&  port_state== 0)
       {
         port_setting = 0;
         rtu_pass_all_on_port(i,0);   // disable port if it's down
         TRACE(TRACE_INFO, "Disabling port %d in RTU [it is down]",i);
       }
       ports_setting = port_setting ? 
                                       (1<<i)    |  ports_setting :
                                       (~(1<<i)) &  ports_setting ;
   }
   usleep(100);
   tru_read_status(&bank, &ports_up, &ports_stable_up,0);

   // port state masks
   port_down     = ((~ports_up) & (~ports_stable_up)) & TRU_ALL_WORKS;
   port_up       =    ports_up  &   ports_stable_up   & TRU_ALL_WORKS;
   port_woke_up  = ((~ports_up) &   ports_stable_up)  ;
   


   /**************************** ports roles setting  ***********************************/
   if(input_active_port < -1)
   {
      TRACE(TRACE_INFO, "rtu_update [mode=%d]: ports_up=0x%x, ports_stable_up=0x%x, "
                        "port_woke_up=0x%x (no automatic active port switching)",
                         input_active_port, ports_up,ports_stable_up,port_woke_up);
   }
   else if(input_active_port == -1) 
   {
      TRACE(TRACE_INFO, "rtu_update [mode=%d]: port_down=0x%x, port_up=0x%x, port_woke_up=0x%x,"
                        "active_port=%d [remembered=%d], backup_port=%d",
                         input_active_port, port_down,port_up,port_woke_up, active_port,
                         remembered_active_port, backup_port);
      if((active_port == 1) && (port_down == TRU_PORT_1) && (port_up == TRU_PORT_2))
      {
         active_port = 2;
         backup_port = 1;       
         change_active_port = 1;
      }
      else if((active_port == 2) && (port_down == TRU_PORT_2) && (port_up == TRU_PORT_1))
      {
         active_port = 1;
         backup_port = 2;      
         change_active_port = 1;       
      }
      else 
         change_active_port = 0;
   }
   else if(input_active_port >= 0)
   {
      if(input_active_port == 1) 
      {
         active_port = 1;
         backup_port = 2;      
         change_active_port = 1;       
      }  
      if(input_active_port == 2) 
      {
         active_port = 2;
         backup_port = 1;      
         change_active_port = 1;       
      }  
   }
   if(change_active_port)
      tru_set_port_roles(active_port,backup_port);
   
   /*****************************************************************************************/
   
   for(i=0; i <= MAX_PORT; i++)
   {
      if(0x1 & (port_woke_up>>i))
      {
         rtu_pass_all_on_port(i,1);      
         TRACE(TRACE_INFO, "Wake up port %d",i); 
      }
   }   
}

// tru update
static void *rtu_daemon_truud_process(void *arg)
{
    int argument = (int)arg;
    argument = - argument ;
//     TRACE(TRACE_INFO, "-----------\n\n\nrtu_daemon arg %d\n\n\n-----------",argument);
    while(1){
        tru_update_ports_state(argument);
        sleep(2);
    }
    return NULL;
}

/**
 * \brief Periodically removes the filtering database old entries.
 *
 */
static void *rtu_daemon_aging_process(void *arg)
{

    while(1) {
				rtu_update_ports_state();    		
        rtu_fd_flush();
        sleep(1);
    }

    return NULL;
}


/**
 * \brief Handles WRIPC requests.
 * Currently used to dump the filtering database contents when requested by
 * external processes.
 *
 */
static void *rtu_daemon_wripc_process(void *arg)
{
    while(1){
        rtud_handle_wripc();
        usleep(10000);
    }
    return NULL;
}



/**
 * \brief Handles the learning process. 
 * @return error code
 */
static int rtu_daemon_learning_process()
{
    int err, i, port_down;                            
    struct rtu_request req;             // Request read from learning queue
    uint32_t port_map;                  // Destination port map
    uint16_t vid;                       // VLAN identifier

    while(1){
        // Serve pending unrecognised request
        err = rtu_read_learning_queue(&req);
        if (!err) {
            TRACE(
                TRACE_INFO,
                "ureq: port %d src %s VID %d priority %d", 
                req.port_id, 
                mac_to_string(req.src),
                req.has_vid  ? req.vid:0,
                req.has_prio ? req.prio:0
            );
            
						for(port_down=i=0; i<=MAX_PORT;i++)
							if(port_state[i].in_use && port_state[i].hw_index == req.port_id && !port_state[i].is_up)
							{
								port_down = 1;
								break;
							}
							
						/* don't learn on ports that are down (FIFO tail?) */
            if(port_down)
            	continue;
            // If req has no VID, use 0 (untagged packet)
            vid      = req.has_vid ? req.vid:0;
            port_map = (1 << req.port_id);
            // create or update entry at filtering database
            err = rtu_fd_create_entry(req.src, vid, port_map, DYNAMIC);
						err= 0;
            if (err == -ENOMEM) {
                // TODO remove oldest entries (802.1D says you MAY do it)
                TRACE(TRACE_INFO, "filtering database full\n");
            } else if (err) {
                TRACE(TRACE_INFO, "create entry: err %d\n", err);
                break;
            }
        } else {
            TRACE(TRACE_INFO,"read learning queue: err %d\n", err);
        }
    }
    return err;
}


/**
 * \brief RTU set up. 
 * Initialises routing table cache and RTU at hardware.
 * @param poly hash polinomial.
 * @param aging_time Aging time in seconds.
 * @return error code.
 */
static int rtu_daemon_init(uint16_t poly, unsigned long aging_time, int unrec_behavior,
           int static_entries, int tru_enabled)
{
    int i, err;

    // init RTU HW
    TRACE(TRACE_INFO, "init rtu hardware.");
    err   = rtu_init();
    if(err)
        return err;    
    err  = rtux_init();
    if(err)
        return err;   
    
    err  = tru_init(tru_enabled);
    if(err)
        return err;    
    
    // disable RTU
    TRACE(TRACE_INFO, "disable rtu.");
    rtu_disable();

    // init configuration for ports
    TRACE(TRACE_INFO, "init port config.");
    for(i = MIN_PORT; i <= MAX_PORT; i++) {
        // MIN_PORT <= port <= MAX_PORT, thus no err returned

        err = rtu_learn_enable_on_port(i,1);
        err = rtu_pass_all_on_port(i,1);
        err = rtu_pass_bpdu_on_port(i,0);
        err = rtu_set_fixed_prio_on_port(i,0);
        err = rtu_set_unrecognised_behaviour_on_port(i,unrec_behavior);
    }

    ///////////////// RTU eXtension ///////
//     rtu_ext_simple_test();
    // ////////////////////////////////////////

    // init filtering database
    TRACE(TRACE_INFO, "init fd.");
    err = rtu_fd_init(poly, aging_time);
    if (err) 
        return err;

    // create static filtering entries
    if(static_entries)
      err = rtu_create_static_entries();
    if(err)
        return err;

    // turn on RTU
    TRACE(TRACE_INFO, "enable rtu.");
    rtu_enable();

    rtud_init_exports();

    return err;
}

/**
 * \brief RTU shutdown.
 */
static void rtu_daemon_destroy()
{ 
    // Threads stuff
    pthread_cancel(wripc_process);
    pthread_cancel(aging_process);
//     pthread_cancel(truud_process);

    // Turn off RTU
    rtu_disable();
    rtu_exit();
}

void sigint(int signum) {
    rtu_daemon_destroy();
    exit(0);
}

/**
 * \brief Starts up the learning and aging processes.
 */ 
int main(int argc, char **argv)
{
	int op, err;
    char *s, *name, *optstring;			
    int run_as_daemon        = 0;
    uint16_t poly            = HW_POLYNOMIAL_CCITT;  // Hash polinomial
    unsigned long aging_res  = DEFAULT_AGING_RES;    // Aging resolution [sec.]
    unsigned long aging_time = DEFAULT_AGING_TIME;   // Aging time       [sec.]
    int truud_thread_run     = 0;
    int config_mode          = 0;
    int static_entries       = 1; // yes by default
    int unrec_behavior       = 1; //broadcast by default
    int tru_enabled          = 1; // TRU disabled by default
    trace_log_stderr();

    if (argc > 1) {
        // Strip out path from argv[0] if exists, and extract command name
        for (name = s = argv[0]; s[0]; s++) {
            if (s[0] == '/' && s[1]) {
                name = &s[1];
            }
        }
        // Parse daemon options
        optstring = "?dhp:r:t:u:x:o:y:v:c:";
        while ((op = getopt(argc, argv, optstring)) != -1) {
            switch(op) {
            case 'd':
                run_as_daemon = 1;
                break;
            case 'h':
                usage(name);                
            case 'p':
                if (strcmp(optarg, "CCITT") == 0) {
                    poly = HW_POLYNOMIAL_CCITT;
                } else if (strcmp(optarg, "IBM") == 0) {
                    poly = HW_POLYNOMIAL_IBM;
                } else if (strcmp(optarg, "DECT") == 0) {
                    poly = HW_POLYNOMIAL_DECT;
                } else {
                    fprintf(stderr, "Invalid polynomial\n");
                    usage(name);
                }
                break;
            case 'r':
                if ((aging_res = atol(optarg)) <= 0) {
                    fprintf(stderr, "Invalid aging resolution\n");
                    usage(name);
                }
                break;
            case 't':
                aging_time = atol(optarg);
                if ((aging_time < MIN_AGING_TIME) || 
                    (aging_time > MAX_AGING_TIME)) {
                    fprintf(stderr, "Invalid aging time\n");
                    usage(name);
                }
                break;
            case 'u':
                tru_set_life(optarg);
                break;
            case 'x':
                rtux_set_life(optarg);
                break;
            case 'o':
                rtu_set_life(optarg);
                break;
            case 'y':
                tru_update_ports_state(atol(optarg));
                 exit(1);
                break;
            case 'v':
               truud_thread_run = atol(optarg);
               tru_enabled      = 1; // tru enabled
               fprintf(stderr, "TRU thread: %d\n", truud_thread_run);
               break;
            case 'c':
               config_mode = atol(optarg);
               fprintf(stderr, "\n>>>>>>>>>>>>>>>>>> Config-mode: %d <<<<<<<<<<<<<<<<\n", config_mode);
               if(config_mode == 1)
               {
                 truud_thread_run = 1;
                 tru_enabled      = 1;
                 fprintf(stderr, "\nTRU update with pref-configured active/backup port"
                                 "(1 is active, 2 is backup)\n");
               }
               else if(config_mode == 2)
               {
                 truud_thread_run = 2;
                 static_entries   = 0;
                 unrec_behavior   = 0;
                 tru_enabled      = 0;
                 fprintf(stderr, "\nNo static entrie, drop unrecognized, TRU disabled\n");               
               }                  
               fprintf(stderr, "\n>>>>>>>>>>>>>>>>>>>>>>>  <<<<<<<<<<<<<<<<<<<<<<<<<<\n");
               break;
            case '?':
            default:
                usage(name);
            }
        }
    }

    // Initialise RTU. 
    if((err = rtu_daemon_init(poly, aging_time,unrec_behavior,static_entries,tru_enabled)) < 0) {
        rtu_daemon_destroy();
        return err;
    }

	// Register signal handler
	signal(SIGINT, sigint);

    // daemonize _before_ creating threads
    if(run_as_daemon) 
        daemonize();

    // Start up aging process and auxiliary WRIPC thread
//     if ((err = pthread_create(&aging_process, NULL, rtu_daemon_aging_process, (void *) aging_res)) ||
//         (err = pthread_create(&wripc_process, NULL, rtu_daemon_wripc_process, NULL))               ||
//         (err = pthread_create(&truud_process, NULL, rtu_daemon_truud_process, NULL))) {
//         rtu_daemon_destroy();
//         return err;
//     }

    if ((err = pthread_create(&aging_process, NULL, rtu_daemon_aging_process, (void *) aging_res)) ||
        (err = pthread_create(&wripc_process, NULL, rtu_daemon_wripc_process, NULL))) {
        rtu_daemon_destroy();
        return err;
    }
    if(truud_thread_run)
    {
       if(err = pthread_create(&truud_process, NULL, rtu_daemon_truud_process, (void * ) truud_thread_run))
       {
          pthread_cancel(truud_process);
          return err;
       }
    }
    // Start up learning process.
    err = rtu_daemon_learning_process();
    // On error, release RTU resources
    rtu_daemon_destroy();
	return err;
}


