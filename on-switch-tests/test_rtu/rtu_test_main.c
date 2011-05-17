/*
-------------------------------------------------------------------------------
-- Title      : Routing Table Unit (RTU) test application
-- Project    : White Rabbit Switch
-------------------------------------------------------------------------------
-- File       : rtu_test_main.c
-- Authors    : Maciej Lipinski (maciej.lipinski@cern.ch)
-- Company    : CERN BE-CO-HT
-- Created    : 2010-06-30
-- Last update: 2010-06-30
-- Description:

Main file of application testing RTU unit

application constist of:
  rtu_test_main.c       - main()
  rtu_tset_data.c       - data needed to peformed test is defined there, in particular: 
                            * content of memories: htab (zbt sram), hcam, agr_htab, vlan_tab
                            * requests to be used to test RTU 
  rtu_test.c            - here are two kinds of functions used in the rtu_test_main:
                            1) wrappers - functions calling simulation and performing
                               hardware access. In other words, a function in rtu_test.c calles its
                               equivalent for simulation and hardware,i.e.:
                                                  rtu_set()
                                                     |
                                            ---------------------
                                            |                   |
                                        rtu_sim_set()      rtu_hw_set()
                                        
                             2) result presentation - functions used to compare data received 
                                from rtu hardware unit and simulation rtu_match() functions and
                                presenting the results (rtu_dump_****)
                                
    rtu_common.c        - should contain functions common for simulation and hardware access.
                           In fact, it contains only hash-address-calculation connected functions.
                            
    rtu_sim.h           - data types used in simulation, i.e. to store data (simulate zbt sram, etc)
    
    rtu_sim.c           - contain all the functions connected with simulation of RTU unit, in particular:
                             * functions implementing filtering algorithm
                             * functions enabling to store data in tables/structures simulating memories
   
    rtu_hw.c            - contains functions enabling access to hardware (reading/writing data). The top entity
                          contain CPU-wishbone interface as well as wishbone bridge to enable addressing many 
                          wishbone-something interfaces. Two kinds of accesses to RTU unit:
                              * "normal" communication with RTU unit 
                                    ** it is the communication which is performed during normal 
                                       operation of RTU (and the White Rabbit Switch)
                                    ** it is done through wishbone-RTU interface
                                    
                              * "test" communication with RTU unit
                                    ** this includes feeding RTU unit with requests and receiving
                                       responses from RTU unit
                                    ** during normal operation requests come from endpoints and are also
                                       received by endpoints (which are implemented in HW as well
                                    ** during tests, RTU unit is wrapped into wb_rtu_test_unit.vhd unit
                                       It contains wishbone<->port interface for each port of RTU unit
                                    ** each wishbone<->port includes in_fifo for request and out_fifo for responses
                                    ** there is also 
                                    
                               * "debugging" communication with RTU unit
                                    ** included into RTU unit is wishbone-RTU_debugging interfaces 
                                    ** two kinds of "debugging"
                                        *** storing into output fifo data readout from zbt sram (as is)
                                            it needs to be enabled in global_defs.vhd
                                        *** read/write zbt sram directly - which enables directly
                                            access to zbt_memory
  
 In order to "have fun" with testing RTU unit, just call in the folder:
 ./build.sh run T=wrsw_IP
 
 i.e.: ./build.sh run T=192.168.1.5


*/


#include <stdio.h>
#include <time.h> 
#include <hw/switch_hw.h>
#include "rtu_sim.h"
#include "rtu_test_main.h"
/*
   =============================================================================================================================
						    MAIN TEST FOR RTU
   =============================================================================================================================
*/



/*
it is basically entire test for RTU
*/
int test_rtu(int verbose)
{
	int i = 0;
	int j =0;
	int err=0;
	int req_number;
	int tmp_port_number ;
	int tmp_port_id;
	
	uint8_t htab_active_bank = (uint8_t)ACTIVE_BANK;
	uint8_t hcam_active_bank = (uint8_t)ACTIVE_BANK;
	
	#ifndef RANDOM_DATA
	req_number = 10;
	#else
	req_number = TEST_REQS;
	#endif
	//table with requests
	rtu_request_t  request_tab[MAX_REQUEST_NUMBER];
	
	//table filled with answers from RTU unit in the switch (HW)
	rtu_response_t response_tab_hw[PORT_NUMBER][MAX_REQUEST_NUMBER];
	
	//table filled with answers from matching function (SW)
	rtu_response_t response_tab_sw[PORT_NUMBER][MAX_REQUEST_NUMBER];
	

	if(verbose) printf("\n 1) ==== clearing memories ====\n");
	
	// clean all other mems
	rtu_clean_mems(htab_active_bank,hcam_active_bank);  //rtu_test.c
	clean_output_fifos();
	
	if(verbose) printf("\n 2) ==== some simple tests ====\n");
	
	//test_in_fifos();
	//test_learning();

	if(verbose) printf("\n 3) ===== filling in memories =====\n");
	
	
	/*
	IMPORTANT: 
	Setting active bank 1 for htab & hcam
	We will be testing RTU on bank =0 so to 
	write data for bank =0 we need to switch to 
	bank = 1, since always the unactive bank 
	has write access when active bank has only read access
	*/

	rtu_set_hash_poly(USE_POLYNOMIAL);
	rtu_set_active_hcam_bank((hcam_active_bank == 0 ? 1 : 0));
	rtu_set_active_htab_bank((htab_active_bank == 0 ? 1 : 0));

	
	//filling in filtering table
	rtu_fill_in_htab();   	//rtu_test_data.c
	
	//filling in rest of filtering table
	//rtu_fill_in_hcam(); 	//rtu_test_data.c
	
	//filling in vlan table
	rtu_fill_in_vlan_tab();	//rtu_test_data.c

	if(verbose) printf("\n 4) ==== Basic RTU settings ====\n");	
	
	/*
	IMPORTANT:
	we need to change banks before filling in request table
	it is because macs are read from htab and:
	while writing to htab is done to non-active bank
	      reading from htab is done from active banks
	
	*/
	//configuration to sim/hw RTU unit and ports
	rtu_fill_settings(hcam_active_bank /*hcam_bank*/,htab_active_bank /*htab_bank*/,USE_POLYNOMIAL ); //rtu_test_main.c

	if(verbose) printf("\n 5) ===== filling in test data ==== \n");
	if(verbose) printf("\n    === write requests to table === \n");
	
	if(verbose)
	for(i=0;i<((cam_address[hcam_active_bank]/8) - 1);i++)
	{
	  printf("rtu_cam[%d][0x%x]: 0x%2x%2x%2x%2x%2x%2x , fid: 0x%x \n",hcam_active_bank,i,rtu_cam[hcam_active_bank][i].mac[0],rtu_cam[hcam_active_bank][i].mac[1],rtu_cam[hcam_active_bank][i].mac[2],rtu_cam[hcam_active_bank][i].mac[3],rtu_cam[hcam_active_bank][i].mac[4],rtu_cam[hcam_active_bank][i].mac[5],rtu_cam[hcam_active_bank][i].fid);
	}
	//fill in test table with requests
	rtu_fill_in_req_table(request_tab,req_number);//rtu_test_data.c


	
	if(verbose) printf("\n 6) ======== testing RTU   ===========\n");

	int port_ids[PORT_NUMBER]; // we need to check responses on each port separatelly
		                   // so we count number of responses for each port
				   
	
	
#ifdef HW_TEST_ONE_BY_ONE	
	printf("    \n   | =========     TESTING METHOD: ONE-BY-ONE     ============= |\n");
	printf("       | Testing RTU on one-by-one request base - writing request   |\n");
	printf("       | to a given port -> reading the response from the given port|\n");
	printf("\n       | ===========================================================|\n");
	// ----------------------- first kind of testing : one-by-one request ------------------//
	// write requests to hardware and reading the responses
	// it is done one-by-one
	// TODO: flood input fifo with many requets 
	//       and than read responses
	for(i=0; i < PORT_NUMBER;i++)
	  port_ids[i] = 0;
	
	for(i=0;i<req_number;i++)
	{
	  if(i%10==0) fprintf(stderr, "."); 
	  tmp_port_number = request_tab[i].port_id;
	  tmp_port_id = port_ids[tmp_port_number];
	  rtu_hw_match(request_tab[i], &response_tab_hw[tmp_port_number][tmp_port_id]); //rtu_hw.c
	  port_ids[tmp_port_number] ++;
	}
#else	
	printf("\n   | =========      TESTING METHOD: FLOODING RTU       ============  |\n");
	printf("   | Flooding RTU with all the requests [%4d] on all the ports [%2d] |\n",req_number, (int)PORT_NUMBER);
	printf("   | afterwords, reading requets from all the ports                  |\n");
	printf("\n   | =============================================================== |\n");
	// ----------------------- first kind of testing : flood RTU ------------------//	
	
	for(i=0; i < PORT_NUMBER;i++)
	  port_ids[i] = 0;
	
	//write all requests
	for(i=0;i<req_number;i++)
	{
	  if(i%10==0) fprintf(stderr, "."); 
	  rtu_hw_write_request_entry_from_req_table(request_tab[i]);
	  usleep(1000);
	}

	
	// read responses
	i=0;
	int err_i=0;
	while(i<req_number)
	{
	  //check each port
	  for(j=0;j<PORT_NUMBER;j++)
	  {
	    // check if there is any answer on the port
	    if(rtu_hw_check_if_answer_on_port( j ))
	    {
	       
		tmp_port_id = port_ids[j];
		// readh the answer
		rtu_hw_read_response_from_port(&response_tab_hw[j][tmp_port_id],j);
		//printf("reading: i=%d, port_ids[%d] = %d\n",i,tmp_port_number,port_ids[tmp_port_number]  );
		port_ids[j] ++;
		i++;
	    }
	      
	  }
	
	  if(err_i++ > 100000) {printf("problem with reading responses (hanged on reading hw-responses)\n"); break;}
	
	}
	int reqs =0;
	printf("\n");
	for(i=0;i<PORT_NUMBER;i++)
	{
	  printf("PORT %d : %d requets/responses\n",i,port_ids[i]);
	  reqs +=port_ids[i];
	}
	printf("==============================\n");
	printf("SUM    : %d requets/responses\n",reqs);
#endif	
	fprintf(stderr, "\n"); 
	
	if(verbose) printf("\n 7) ======== simulating RTU   ===========\n");

	// computes requests with software implementation of 
	// RTU algorithm
	for(i=0;i<PORT_NUMBER;i++)
	  port_ids[i] = 0;
	
	
	for(i=0;i<req_number;i++)
	{
	  if(verbose) printf("====== sim_match [%d] =========\n",i);
	  tmp_port_number = request_tab[i].port_id;
	  tmp_port_id = port_ids[tmp_port_number];
	  rtu_sim_match(request_tab[i], &response_tab_sw[tmp_port_number][tmp_port_id],verbose); //rtu_sim.c
	  port_ids[tmp_port_number] ++;
	}
	
	printf("\nTOTAL NUMBER OF FOUND ENTRIES in simulation(htab & hcam): %d\n",number_of_found_entries);
	
	if(verbose) printf("\n 8)======  comparing responses ===== \n");
	
	// go through responses received from simulation and hardware
	// and compare the results
	int err_dump_results = 0;
	j=0;
	
	for(i=0;i<PORT_NUMBER;i++)
	  port_ids[i] = 0;
	
	for(i=0;i<req_number;i++)
	{
	  tmp_port_number = request_tab[i].port_id;
	  tmp_port_id = port_ids[tmp_port_number];
	  err_dump_results += rtu_dump_results(request_tab[i],response_tab_sw[tmp_port_number][tmp_port_id],response_tab_hw[tmp_port_number][tmp_port_id],i);//rtu_test.c
	  port_ids[tmp_port_number] ++;
	}
	  
	if(err_dump_results==0)   
	  if(verbose) printf("\n \t\t ALL results OK\n");
	
	err += err_dump_results;
	
	if(verbose) printf("\n 9)======  read aging mems ======= \n\n");
	
	//to stored words changed in agr_htab
	changed_aging_htab_word_t hw_agr[ARAM_WORDS];
	changed_aging_htab_word_t sim_agr[ARAM_WORDS];
	
	//to store agr_hcam registers' content
	uint32_t sim_agr_hcam;
	uint32_t hw_gr_hcam;

	// reading aging htab
	int agr_htab_cnt = rtu_read_arg_htab_changes(sim_agr,hw_agr);//rtu_test.c

	//reading aging hcam
	rtu_read_arg_hcam(&sim_agr_hcam,&hw_gr_hcam);
	
	
	if(verbose) printf("\n 10)======  comparing aging mems === \n");	
	
	// go through entires which changed in main aging memory 
	// in simulation and hardware 
	// and compare the results
	int err_dump_aging_mems = 0;
	int err_dump_agr_hcam = 0;
	
	for(i=0;i<agr_htab_cnt;i++)
	  err_dump_aging_mems += rtu_dump_aging_mems(sim_agr[i],hw_agr[i] );//rtu_test.c

	if(err_dump_aging_mems==0)   
	  if(verbose) printf("\n \t\t ALL htab aging updates OK\n");	  
	  
	//comparing agr_hcam for simulation and hardware
	err_dump_agr_hcam = rtu_dump_agr_hcam(sim_agr_hcam,hw_gr_hcam);//rtu_test.c
	
	if(err_dump_agr_hcam==0)   
	  if(verbose) printf("\n \t\t ALL hcam aging updates OK\n");
	
	err += (err_dump_aging_mems + err_dump_agr_hcam);
	
	if(verbose) printf("\n 11)======  reading learning queue === \n\n");	

	// to store learning queue read from H/W
	request_fifo_t learning_queue_hw;
	
	// read learning queue from hardware (we already have simulation)
	int queue_cnt = rtu_read_learning_queue(&learning_queue_hw);//rtu_test.c
	
	
	if(verbose) printf("\n 12)======  comparing learning queue === \n");	
	
	// comparing learning queues - go through each entry
	// received from simulation and hardware
	int err_dump_learning_queue = 0;
	
	for(i=0;i<queue_cnt;i++)
	  err_dump_learning_queue += rtu_dump_learning_queue(learning_queue.data[i+1],learning_queue_hw.data[i]);//rtu_test.c

	if(verbose) printf("\nFirst %d entries OK\n",(queue_cnt+err_dump_learning_queue));
	 
	if(err_dump_learning_queue==0)   
	    if(verbose) printf("\n \t\t ALL results OK\n");  
	  
	err += err_dump_learning_queue;
	
	if(verbose) printf("\n 13)======  summing up... === \n");
	
	//if(verbose)
	if(err != 0) rtu_info();
	
	#ifdef DEBUG_HCAM
	rtu_dbg_hcam();
	#endif

	
	return err;

}


//////////////////////////////////////////////////////////////////////////////////////



/*
it is basically entire test for RTU
*/
int test_rtu_cont(int verbose)
{
	int i = 0;
	int j =0;
	int err=0;
	int global_err=0;
	int req_number;
	int tmp_port_number ;
	int tmp_port_id;
	int iterations = 0;
	uint8_t htab_active_bank = (uint8_t)ACTIVE_BANK;
	uint8_t hcam_active_bank = (uint8_t)ACTIVE_BANK;
	
	#ifndef RANDOM_DATA
	req_number = 10;
	#else
	req_number = TEST_REQS;
	#endif
	//table with requests
	rtu_request_t  request_tab[MAX_REQUEST_NUMBER];
	
	//table filled with answers from RTU unit in the switch (HW)
	rtu_response_t response_tab_hw[PORT_NUMBER][MAX_REQUEST_NUMBER];
	
	//table filled with answers from matching function (SW)
	rtu_response_t response_tab_sw[PORT_NUMBER][MAX_REQUEST_NUMBER];
	

	if(verbose) printf("\n 1) ==== clearing memories ====\n");
	
	// clean all other mems
	rtu_clean_mems(htab_active_bank,hcam_active_bank);  //rtu_test.c
	clean_output_fifos();
	
	if(verbose) printf("\n 2) ==== some simple tests ====\n");
	
	//test_in_fifos();
	//test_learning();

	if(verbose) printf("\n 3) ===== filling in memories =====\n");
	
	
	/*
	IMPORTANT: 
	Setting active bank 1 for htab & hcam
	We will be testing RTU on bank =0 so to 
	write data for bank =0 we need to switch to 
	bank = 1, since always the unactive bank 
	has write access when active bank has only read access
	*/

	rtu_set_hash_poly(USE_POLYNOMIAL);
	rtu_set_active_hcam_bank((hcam_active_bank == 0 ? 1 : 0));
	rtu_set_active_htab_bank((htab_active_bank == 0 ? 1 : 0));

//	rtu_set_active_hcam_bank(hcam_active_bank);
//	rtu_set_active_htab_bank(htab_active_bank);

	
	//filling in filtering table
	rtu_fill_in_htab();   	//rtu_test_data.c
	
	//filling in rest of filtering table
	//rtu_fill_in_hcam(); 	//rtu_test_data.c
	
	//filling in vlan table
	rtu_fill_in_vlan_tab();	//rtu_test_data.c

	if(verbose) printf("\n 4) ==== Basic RTU settings ====\n");	

while(iterations < TEST_ITERATIONS)
{
	
	iterations ++;
	/* clean learning queue *
	*************************/
	learning_queue.head = 0;
	learning_queue.tail = MAX_FIFO_SIZE-1;
	learning_queue.count = 0;
	

	/*
	IMPORTANT:
	we need to change banks before filling in request table
	it is because macs are read from htab and:
	while writing to htab is done to non-active bank
	      reading from htab is done from active banks
	
	*/
	#ifdef DEBUG_HCAM  
	rtu_read_hcam_entries();
	#endif	
	
	//configuration to sim/hw RTU unit and ports
	rtu_fill_settings(hcam_active_bank /*hcam_bank*/,htab_active_bank /*htab_bank*/,USE_POLYNOMIAL ); //rtu_test_main.c
	

	
	if(verbose)
	for(i=0;i<((cam_address[hcam_active_bank]/8));i++)
	{
	  printf("rtu_cam[%d][0x%x]: 0x%2x%2x%2x%2x%2x%2x , fid: 0x%x \n",hcam_active_bank,i,rtu_cam[hcam_active_bank][i].mac[0],rtu_cam[hcam_active_bank][i].mac[1],rtu_cam[hcam_active_bank][i].mac[2],rtu_cam[hcam_active_bank][i].mac[3],rtu_cam[hcam_active_bank][i].mac[4],rtu_cam[hcam_active_bank][i].mac[5],rtu_cam[hcam_active_bank][i].fid);
	}	


	
	hcam_active_bank = (hcam_active_bank == 0 ? 1 : 0);
	htab_active_bank = (htab_active_bank == 0 ? 1 : 0);
	
	if(verbose) printf("\n 5) ===== filling in test data ==== \n");
	if(verbose) printf("\n    === write requests to table === \n");
	//fill in test table with requests
	rtu_fill_in_req_table(request_tab,req_number);//rtu_test_data.c


	
	if(verbose) printf("\n 6) ======== testing RTU   ===========\n");

	int port_ids[PORT_NUMBER]; // we need to check responses on each port separatelly
		                   // so we count number of responses for each port
				   
	
	
#ifdef HW_TEST_ONE_BY_ONE	
	printf("\n       | =========     TESTING METHOD: ONE-BY-ONE     ============= |\n");
	printf("       | Testing RTU on one-by-one request base - writing request   |\n");
	printf("       | to a given port -> reading the response from the given port|\n");
	printf("\n       | ===========================================================|\n");
	// ----------------------- first kind of testing : one-by-one request ------------------//
	// write requests to hardware and reading the responses
	// it is done one-by-one
	// TODO: flood input fifo with many requets 
	//       and than read responses
	for(i=0; i < PORT_NUMBER;i++)
	  port_ids[i] = 0;
	
	for(i=0;i<req_number;i++)
	{
	  if(i%10==0) fprintf(stderr, "."); 
	  tmp_port_number = request_tab[i].port_id;
	  tmp_port_id = port_ids[tmp_port_number];
	  rtu_hw_match(request_tab[i], &response_tab_hw[tmp_port_number][tmp_port_id]); //rtu_hw.c
	  port_ids[tmp_port_number] ++;
	}
#else	
	printf("\n   | =========      TESTING METHOD: FLOODING RTU       ============  |\n");
	printf("   | Flooding RTU with all the requests [%4d] on all the ports [%2d] |\n",req_number, (int)PORT_NUMBER);
	printf("   | afterwords, reading requets from all the ports                  |\n");
	printf("\n   | =============================================================== |\n");
	// ----------------------- first kind of testing : flood RTU ------------------//	
	
	for(i=0; i < PORT_NUMBER;i++)
	  port_ids[i] = 0;
	
	//write all requests
	for(i=0;i<req_number;i++)
	{
	  if(i%10==0) fprintf(stderr, "."); 
	  rtu_hw_write_request_entry_from_req_table(request_tab[i]);
	}

	
	// read responses
	i=0;
	int err_i=0;
	while(i<req_number)
	{
	  //check each port
	  for(j=0;j<PORT_NUMBER;j++)
	  {
	    // check if there is any answer on the port
	    if(rtu_hw_check_if_answer_on_port( j ))
	    {
	       
		tmp_port_id = port_ids[j];
		// readh the answer
		rtu_hw_read_response_from_port(&response_tab_hw[j][tmp_port_id],j);
		//printf("reading: i=%d, port_ids[%d] = %d\n",i,tmp_port_number,port_ids[tmp_port_number]  );
		port_ids[j] ++;
		i++;
	    }
	      
	  }
	
	  if(err_i++ > 100000) {printf("problem with reading responses (hanged on reading hw-responses)\n"); break;}
	
	}
#endif		
	int reqs =0;
	printf("\n");
	for(i=0;i<PORT_NUMBER;i++)
	{
	  printf("PORT %d : %d requets/responses\n",i,port_ids[i]);
	  reqs +=port_ids[i];
	}
	printf("==============================\n");
	printf("SUM    : %d requets/responses\n",reqs);

	fprintf(stderr, "\n"); 
	
	if(verbose) printf("\n 7) ======== simulating RTU   ===========\n");

	// computes requests with software implementation of 
	// RTU algorithm
	for(i=0;i<PORT_NUMBER;i++)
	  port_ids[i] = 0;
	
	
	for(i=0;i<req_number;i++)
	{
	  if(verbose) printf("====== sim_match [%d] =========\n",i);
	  tmp_port_number = request_tab[i].port_id;
	  tmp_port_id = port_ids[tmp_port_number];
	  rtu_sim_match(request_tab[i], &response_tab_sw[tmp_port_number][tmp_port_id],verbose); //rtu_sim.c
	  port_ids[tmp_port_number] ++;
	}
	
	printf("\nTOTAL NUMBER OF FOUND ENTRIES in simulation(htab & hcam): %d\n",number_of_found_entries);
	
	if(verbose) printf("\n 8)======  comparing responses ===== \n");
	
	// go through responses received from simulation and hardware
	// and compare the results
	int err_dump_results = 0;
	j=0;
	
	for(i=0;i<PORT_NUMBER;i++)
	  port_ids[i] = 0;
	
	for(i=0;i<req_number;i++)
	{
	  tmp_port_number = request_tab[i].port_id;
	  tmp_port_id = port_ids[tmp_port_number];
	  err_dump_results += rtu_dump_results(request_tab[i],response_tab_sw[tmp_port_number][tmp_port_id],response_tab_hw[tmp_port_number][tmp_port_id],i);//rtu_test.c
	  port_ids[tmp_port_number] ++;
	}
	  
	if(err_dump_results==0)   
	  if(verbose) printf("\n \t\t ALL results OK\n");
	
	err += err_dump_results;
	
	if(verbose) printf("\n 9)======  read aging mems ======= \n\n");
	
	//to stored words changed in agr_htab
	changed_aging_htab_word_t hw_agr[ARAM_WORDS];
	changed_aging_htab_word_t sim_agr[ARAM_WORDS];
	
	//to store agr_hcam registers' content
	uint32_t sim_agr_hcam;
	uint32_t hw_gr_hcam;

	// reading aging htab
	int agr_htab_cnt = rtu_read_arg_htab_changes(sim_agr,hw_agr);//rtu_test.c

	//reading aging hcam
	rtu_read_arg_hcam(&sim_agr_hcam,&hw_gr_hcam);
	
	
	if(verbose) printf("\n 10)======  comparing aging mems === \n");	
	
	// go through entires which changed in main aging memory 
	// in simulation and hardware 
	// and compare the results
	int err_dump_aging_mems = 0;
	int err_dump_agr_hcam = 0;
	
	for(i=0;i<agr_htab_cnt;i++)
	  err_dump_aging_mems += rtu_dump_aging_mems(sim_agr[i],hw_agr[i] );//rtu_test.c

	if(err_dump_aging_mems==0)   
	  if(verbose) printf("\n \t\t ALL htab aging updates OK\n");	  
	  
	//comparing agr_hcam for simulation and hardware
	err_dump_agr_hcam = rtu_dump_agr_hcam(sim_agr_hcam,hw_gr_hcam);//rtu_test.c
	
	if(err_dump_agr_hcam==0)   
	  if(verbose) printf("\n \t\t ALL hcam aging updates OK\n");
	
	err += (err_dump_aging_mems + err_dump_agr_hcam);
	
	if(verbose) printf("\n 11)======  reading learning queue === \n\n");	

	// to store learning queue read from H/W
	request_fifo_t learning_queue_hw;
	
	// read learning queue from hardware (we already have simulation)
	int queue_cnt = rtu_read_learning_queue(&learning_queue_hw);//rtu_test.c
	
	
	if(verbose) printf("\n 12)======  comparing learning queue === \n");	
	
	// comparing learning queues - go through each entry
	// received from simulation and hardware
	int err_dump_learning_queue = 0;
	
	for(i=0;i<queue_cnt;i++)
	  err_dump_learning_queue += rtu_dump_learning_queue(learning_queue.data[i+1],learning_queue_hw.data[i]);//rtu_test.c

	if(verbose) printf("\nFirst %d entries OK\n",(queue_cnt+err_dump_learning_queue));
	 
	if(err_dump_learning_queue==0)   
	    if(verbose) printf("\n \t\t ALL results OK\n");  
	  
	err += err_dump_learning_queue;
	
	printf("================  ERR [iteration = %d] =  %d  =====================\n",iterations, -err);
	//if(verbose) 
	if(-err) rtu_info();
	 
	if(verbose) printf("================  ==========================  =====================\n");
	//filling in filtering table
	
	//zero nasty global variables
	number_of_found_entries = 0;
	hash_address_writen_cnt = 0;
	
	
	//srand(0);
	rtu_fill_in_htab();   	//rtu_test_data.c
	if(verbose) if(-err) clean_output_fifos();
	global_err = global_err + err;
	err = 0;
	
	printf("global err: %d\n",global_err);
}//while	
	
	if(verbose) printf("\n 13)======  summing up... === \n");

	#ifdef DEBUG_HCAM
	rtu_dbg_hcam();
	#endif

	
	return global_err;

}

/////////////////////////// continuous work //////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////



/*
it is basically entire test for RTU
*/
int test_rtu_long_test(int verbose)
{
	int i = 0;
	int z = 0;
	int j =0;
	int err=0;
	int global_err=0;
	int req_number;
	int tmp_port_number ;
	int tmp_port_id;
	int iterations = 0;
	uint8_t htab_active_bank = (uint8_t)ACTIVE_BANK;
	uint8_t hcam_active_bank = (uint8_t)ACTIVE_BANK;
	int whatever = 0;
	int result=0;
	int stime;
	#ifndef RANDOM_DATA
	req_number = 10;
	#else
	req_number = TEST_REQS;
	#endif
	//table with requests
	rtu_request_t  request_tab[MAX_REQUEST_NUMBER];
	
	//table filled with answers from RTU unit in the switch (HW)
	rtu_response_t response_tab_hw[PORT_NUMBER][MAX_REQUEST_NUMBER];
	
	//table filled with answers from matching function (SW)
	rtu_response_t response_tab_sw[PORT_NUMBER][MAX_REQUEST_NUMBER];
	

	if(verbose) printf("\n 1) ==== clearing memories ====\n");
	
	// clean all other mems
	//rtu_clean_mems(htab_active_bank,hcam_active_bank);  //rtu_test.c
	//clean_output_fifos();
	
	if(verbose) printf("\n 2) ==== some simple tests ====\n");
	
	//test_in_fifos();
	//test_learning();

	if(verbose) printf("\n 3) ===== filling in memories =====\n");
	
	
	/*
	IMPORTANT: 
	Setting active bank 1 for htab & hcam
	We will be testing RTU on bank =0 so to 
	write data for bank =0 we need to switch to 
	bank = 1, since always the unactive bank 
	has write access when active bank has only read access
	*/

	
	rtu_set_hash_poly(USE_POLYNOMIAL);
	rtu_set_active_hcam_bank((hcam_active_bank == 0 ? 1 : 0));
	rtu_set_active_htab_bank((htab_active_bank == 0 ? 1 : 0));

//	rtu_set_active_hcam_bank(hcam_active_bank);
//	rtu_set_active_htab_bank(htab_active_bank);

	
	//filling in filtering table
	//rtu_fill_in_htab();   	//rtu_test_data.c

	
	//filling in vlan table
	//rtu_fill_in_vlan_tab();	//rtu_test_data.c

	if(verbose) printf("\n 4) ==== Basic RTU settings ====\n");	

	
	
#ifdef MULTI_LOOP_TEST
  while(whatever < MULTI_LOOP_TEST_NUMBER)
#endif

#ifdef CONTINUOUS_TEST
while(1)
#endif
{
	whatever++;
	iterations = 0;
	
	
	while(iterations < TEST_ITERATIONS)
	{
		
		if(iterations == 0) // first
		{
		
		//in case we have error and the test needs to be repeated with the 
		//same data, the only way to do it is to supply the same seed
		// this is why we print out seed which is based on local time		  
		#ifdef CUSTOME_SEED
			stime = CUSTOME_SEED;
			srand(stime);
			printf("Custome seed of rand(): %d\n", stime);

		#else

			#ifdef TIME_DRIVEN_RANDOM
				{
				  long ltime = time(NULL);
				  stime = (unsigned) ltime/2;
				  //stime = 6604;
				  srand(stime);
				  printf("Seed of rand() based on current time: %d\n", stime);
				}
				
			#else
				{
				  printf("   | - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - |\n");
				  printf("   | Seed not defined - each time test program is run, the test data is the same.|\n");
				  printf("   |          To get different test data each time test program is run,          |\n");
				  printf("   |           uncomment #define TIME_DRIVEN_RANDOM in rtu_test_main.h           |\n");
				  printf("   | - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - |\n");
				}
				srand(0);
			#endif	

		#endif	
		    
	            /*
		    this one is tricky, the current value of *_active_bank is
		    ahead, so it is opposite to what is set, so we need 
		    to do below magic
		    */
		    rtu_clean_mems(  (htab_active_bank == 0 ? 1 : 0) ,\
		                     (hcam_active_bank == 0 ? 1 : 0) );  
		    clean_output_fifos();
		    
		    //just in case
		    rtu_set_active_hcam_bank((hcam_active_bank == 0 ? 1 : 0));
		    rtu_set_active_htab_bank((htab_active_bank == 0 ? 1 : 0));
		    
		    for(z=0;z<RTU_ENTRIES;z++)
			hash_addresses[z] = 0;
			
		    hash_address_writen_cnt = 0;

    		    cam_address[0] = 0;
		    cam_address[1] = 0;

		    // clean aging memory for htab
		    //rtu_hw_clear_arg_htab();
		    
		    //  for(z=0;z< ARAM_WORDS; z++)
		    // rtu_agr_htab[z]= 0x00000000;
		    
		    // clean aging memory for hcam
		    //rtu_hw_write_agr_hcam(0x00000000); 
		    //printf("after cleaning agr_hcam = 0x%x\n",rtu_hw_read_arg_hcam());
		    //rtu_agr_hcam= 0x00000000;
		    
		    //rtu_clean_mems(htab_active_bank,hcam_active_bank); 
		    rtu_fill_in_htab();   	//rtu_test_data.c
		    rtu_fill_in_vlan_tab();	//rtu_test_data.c
		    
		}
		else
		{
		  rtu_fill_in_htab();   	//rtu_test_data.c
		}
		
		
		iterations ++;
		/* clean learning queue *
		*************************/
		learning_queue.head = 0;
		learning_queue.tail = MAX_FIFO_SIZE-1;
		learning_queue.count = 0;
		

		/*
		IMPORTANT:
		we need to change banks before filling in request table
		it is because macs are read from htab and:
		while writing to htab is done to non-active bank
		      reading from htab is done from active banks
		
		*/
		#ifdef DEBUG_HCAM  
		rtu_read_hcam_entries();
		#endif	
		
		//configuration to sim/hw RTU unit and ports
		rtu_fill_settings(hcam_active_bank /*hcam_bank*/,htab_active_bank /*htab_bank*/,USE_POLYNOMIAL ); //rtu_test_main.c
		

		
		if(verbose)
		for(i=0;i<((cam_address[hcam_active_bank]/8));i++)
		{
		  printf("rtu_cam[%d][0x%x]: 0x%2x%2x%2x%2x%2x%2x , fid: 0x%x \n",hcam_active_bank,i,rtu_cam[hcam_active_bank][i].mac[0],rtu_cam[hcam_active_bank][i].mac[1],rtu_cam[hcam_active_bank][i].mac[2],rtu_cam[hcam_active_bank][i].mac[3],rtu_cam[hcam_active_bank][i].mac[4],rtu_cam[hcam_active_bank][i].mac[5],rtu_cam[hcam_active_bank][i].fid);
		}	


		
		hcam_active_bank = (hcam_active_bank == 0 ? 1 : 0);
		htab_active_bank = (htab_active_bank == 0 ? 1 : 0);
		
		if(verbose) printf("\n 5) ===== filling in test data ==== \n");
		if(verbose) printf("\n    === write requests to table === \n");
		//fill in test table with requests
		rtu_fill_in_req_table(request_tab,req_number);//rtu_test_data.c


		
		if(verbose) printf("\n 6) ======== testing RTU   ===========\n");

		int port_ids[PORT_NUMBER]; // we need to check responses on each port separatelly
					  // so we count number of responses for each port
					  
		
		
	#ifdef HW_TEST_ONE_BY_ONE	
		printf("\n       | =========     TESTING METHOD: ONE-BY-ONE     ============= |\n");
		printf("       | Testing RTU on one-by-one request base - writing request   |\n");
		printf("       | to a given port -> reading the response from the given port|\n");
		printf("\n       | ===========================================================|\n");
		// ----------------------- first kind of testing : one-by-one request ------------------//
		// write requests to hardware and reading the responses
		// it is done one-by-one
		// TODO: flood input fifo with many requets 
		//       and than read responses
		for(i=0; i < PORT_NUMBER;i++)
		  port_ids[i] = 0;
		
		for(i=0;i<req_number;i++)
		{
		  if(i%10==0) fprintf(stderr, "."); 
		  tmp_port_number = request_tab[i].port_id;
		  tmp_port_id = port_ids[tmp_port_number];
		  rtu_hw_match(request_tab[i], &response_tab_hw[tmp_port_number][tmp_port_id]); //rtu_hw.c
		  port_ids[tmp_port_number] ++;
		}
	#else	
		printf("\n   | =========      TESTING METHOD: FLOODING RTU       ============  |\n");
		printf("   | Flooding RTU with all the requests [%4d] on all the ports [%2d] |\n",req_number, (int)PORT_NUMBER);
		printf("   | afterwords, reading requets from all the ports                  |\n");
		printf("\n   | =============================================================== |\n");
		// ----------------------- first kind of testing : flood RTU ------------------//	
		
		for(i=0; i < PORT_NUMBER;i++)
		  port_ids[i] = 0;
		
		//write all requests
		for(i=0;i<req_number;i++)
		{
		  if(i%10==0) fprintf(stderr, "."); 
		  rtu_hw_write_request_entry_from_req_table(request_tab[i]);
		}

		
		// read responses
		i=0;
		int err_i=0;
		while(i<req_number)
		{
		  //check each port
		  for(j=0;j<PORT_NUMBER;j++)
		  {
		    // check if there is any answer on the port
		    if(rtu_hw_check_if_answer_on_port( j ))
		    {
		      
			tmp_port_id = port_ids[j];
			// readh the answer
			rtu_hw_read_response_from_port(&response_tab_hw[j][tmp_port_id],j);
			//printf("reading: i=%d, port_ids[%d] = %d\n",i,tmp_port_number,port_ids[tmp_port_number]  );
			port_ids[j] ++;
			i++;
		    }
		      
		  }
		
		  if(err_i++ > 100000) {printf("problem with reading responses (hanged on reading hw-responses)\n"); break;}
		
		}
	#endif		
		int reqs =0;
		printf("\n");
		for(i=0;i<PORT_NUMBER;i++)
		{
		  printf("PORT %d : %d requets/responses\n",i,port_ids[i]);
		  reqs +=port_ids[i];
		}
		printf("==============================\n");
		printf("SUM    : %d requets/responses\n",reqs);

		fprintf(stderr, "\n"); 
		
		if(verbose) printf("\n 7) ======== simulating RTU   ===========\n");

		// computes requests with software implementation of 
		// RTU algorithm
		for(i=0;i<PORT_NUMBER;i++)
		  port_ids[i] = 0;
		
		
		for(i=0;i<req_number;i++)
		{
		  if(verbose) printf("====== sim_match [%d] =========\n",i);
		  tmp_port_number = request_tab[i].port_id;
		  tmp_port_id = port_ids[tmp_port_number];
		  rtu_sim_match(request_tab[i], &response_tab_sw[tmp_port_number][tmp_port_id],verbose); //rtu_sim.c
		  port_ids[tmp_port_number] ++;
		}
		
		printf("\nTOTAL NUMBER OF FOUND ENTRIES in simulation(htab & hcam): %d\n",number_of_found_entries);
		
		if(verbose) printf("\n 8)======  comparing responses ===== \n");
		
		// go through responses received from simulation and hardware
		// and compare the results
		int err_dump_results = 0;
		j=0;
		
		for(i=0;i<PORT_NUMBER;i++)
		  port_ids[i] = 0;
		
		for(i=0;i<req_number;i++)
		{
		  tmp_port_number = request_tab[i].port_id;
		  tmp_port_id = port_ids[tmp_port_number];
		  err_dump_results += rtu_dump_results(request_tab[i],response_tab_sw[tmp_port_number][tmp_port_id],response_tab_hw[tmp_port_number][tmp_port_id],i);//rtu_test.c
		  port_ids[tmp_port_number] ++;
		}
		  
		if(err_dump_results==0)   
		  if(verbose) printf("\n \t\t ALL results OK\n");
		
		err += err_dump_results;
		
		if(verbose) printf("\n 9)======  read aging mems ======= \n\n");
		
		//to stored words changed in agr_htab
		changed_aging_htab_word_t hw_agr[ARAM_WORDS];
		changed_aging_htab_word_t sim_agr[ARAM_WORDS];
		
		//to store agr_hcam registers' content
		uint32_t sim_agr_hcam;
		uint32_t hw_gr_hcam;

		// reading aging htab
		int agr_htab_cnt = rtu_read_arg_htab_changes(sim_agr,hw_agr);//rtu_test.c

		//reading aging hcam
		rtu_read_arg_hcam(&sim_agr_hcam,&hw_gr_hcam);
		
		
		if(verbose) printf("\n 10)======  comparing aging mems === \n");	
		
		// go through entires which changed in main aging memory 
		// in simulation and hardware 
		// and compare the results
		int err_dump_aging_mems = 0;
		int err_dump_agr_hcam = 0;
		
		for(i=0;i<agr_htab_cnt;i++)
		  err_dump_aging_mems += rtu_dump_aging_mems(sim_agr[i],hw_agr[i] );//rtu_test.c

		if(err_dump_aging_mems==0)   
		  if(verbose) printf("\n \t\t ALL htab aging updates OK\n");	  
		  
		//comparing agr_hcam for simulation and hardware
		err_dump_agr_hcam = rtu_dump_agr_hcam(sim_agr_hcam,hw_gr_hcam);//rtu_test.c
		
		if(err_dump_agr_hcam==0)   
		  if(verbose) printf("\n \t\t ALL hcam aging updates OK\n");
		
		err += (err_dump_aging_mems + err_dump_agr_hcam);
		
		if(verbose) printf("\n 11)======  reading learning queue === \n\n");	

		// to store learning queue read from H/W
		request_fifo_t learning_queue_hw;
		
		// read learning queue from hardware (we already have simulation)
		int queue_cnt = rtu_read_learning_queue(&learning_queue_hw);//rtu_test.c
		
		
		if(verbose) printf("\n 12)======  comparing learning queue === \n");	
		
		// comparing learning queues - go through each entry
		// received from simulation and hardware
		int err_dump_learning_queue = 0;
		
		for(i=0;i<queue_cnt;i++)
		  err_dump_learning_queue += rtu_dump_learning_queue(learning_queue.data[i+1],learning_queue_hw.data[i]);//rtu_test.c

		if(verbose) printf("\nFirst %d entries OK\n",(queue_cnt+err_dump_learning_queue));
		
		if(err_dump_learning_queue==0)   
		    if(verbose) printf("\n \t\t ALL results OK\n");  
		  
		err += err_dump_learning_queue;
		
		printf("================  ERR [iteration = %d] =  %d  =====================\n",iterations, -err);
		//if(verbose) 
		if(-err) rtu_info();
		
		if(verbose) printf("================  ==========================  =====================\n");
		//filling in filtering table
		
		//zero nasty global variables
		number_of_found_entries = 0;
		hash_address_writen_cnt = 0;
		
		
		//srand(0);
// 		if(iterations == TEST_ITERATIONS) // last
// 		{
// 		
// 				  
// 		#ifdef CUSTOME_SEED
// 			stime = CUSTOME_SEED;
// 			srand(stime);
// 			printf("Custome seed of rand(): %d\n", stime);
// 
// 		#else
// 
// 			#ifdef TIME_DRIVEN_RANDOM
// 				{
// 				  long ltime = time(NULL);
// 				  stime = (unsigned) ltime/2;
// 				  //stime = 6604;
// 				  srand(stime);
// 				  printf("Seed of rand() based on current time: %d\n", stime);
// 				}
// 				
// 			#else
// 				{
// 				  printf("   | - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - |\n");
// 				  printf("   | Seed not defined - each time test program is run, the test data is the same.|\n");
// 				  printf("   |          To get different test data each time test program is run,          |\n");
// 				  printf("   |           uncomment #define TIME_DRIVEN_RANDOM in rtu_test_main.h           |\n");
// 				  printf("   | - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - |\n");
// 				}
// 				srand(0);
// 			#endif	
// 
// 		#endif		  
// 		    for(z=0;z<RTU_ENTRIES/4;z++)
// 			hash_addresses[z] = 0;
// 		    
// 		    hash_address_writen_cnt = 0;
// 
//     		    cam_address[0] = 0;
// 		    cam_address[1] = 0;
// 
// 		    // clean aging memory for htab
// 		    rtu_hw_clear_arg_htab();
// 		    
// 		      for(z=0;z< ARAM_WORDS; z++)
// 			  rtu_agr_htab[z]= 0x00000000;
// 		    
// 		    // clean aging memory for hcam
// 		    rtu_hw_write_agr_hcam(0x00000000); 
// 		    printf("after cleaning agr_hcam = 0x%x\n",rtu_hw_read_arg_hcam());
// 		    rtu_agr_hcam= 0x00000000;
// 		    
// 		    //rtu_clean_mems(htab_active_bank,hcam_active_bank); 
// 		    rtu_fill_in_htab();   	//rtu_test_data.c
// 		    rtu_fill_in_vlan_tab();	//rtu_test_data.c
// 		    
// 		}
// 		else
// 		{
// 		  rtu_fill_in_htab();   	//rtu_test_data.c
// 		}
		
		if(verbose) if(-err) clean_output_fifos();
		global_err = global_err + err;
		err = 0;
		
		printf("global err: %d\n",global_err);
	}//while	
	
	summary(global_err);
	
	result =+global_err;
	global_err = 0;
}	

	if(verbose) printf("\n 13)======  summing up... === \n");

	#ifdef DEBUG_HCAM
	rtu_dbg_hcam();
	#endif

	
	return result;

}




/////////////////////////////////////////////////////////////////////////////////////////







/*
   =============================================================================================================================
							      MAIN 
   =============================================================================================================================
*/

main()
{
  
	
	int err = 0;
	int iter_error = 0;
	int i;
	int iter_number = 10;
	int stime;
	struct tm *local;
        time_t t;
	
	// hardware magic
	printf("1  -----------------------------------\n");
	trace_log_stderr();
	printf("2  -----------------------------------\n");
	shw_request_fpga_firmware(FPGA_ID_MAIN, "rtu_test");
	printf("3  -----------------------------------\n");
	shw_init();
	printf("4  -----------------------------------\n");

	// nice start :)
	
 	printf("\t\t=\n");
 	printf("\t\t  =\n");
 	printf("\t\t    =\n");
 	printf("\t\t      =\n");
 	printf("\t\t        =\n");
 	printf("\t\t          ====== LET's START FUN ===== \n");
 	printf("\t\t                                       = \n");
 	printf("\t\t                                         = \n");
 	printf("\t\t                                           = \n");
 	printf("\t\t                                             = \n");
 	printf("\t\t                                               = \n");
 	printf("\t\t                                                 = \n\n\n\n");	
	
// 	//in case we have error and the test needs to be repeated with the 
// 	//same data, the only way to do it is to supply the same seed
// 	// this is why we print out seed which is based on local time
// 
// #ifdef CUSTOME_SEED
// 	stime = CUSTOME_SEED;
// 	srand(stime);
// 	printf("Custome seed of rand(): %d\n", stime);
// 
// #else
// 
// 	#ifdef TIME_DRIVEN_RANDOM
// 		{
// 		  long ltime = time(NULL);
// 		  stime = (unsigned) ltime/2;
// 		  //stime = 6604;
// 		  srand(stime);
// 		  printf("Seed of rand() based on current time: %d\n", stime);
// 		}
// 		
// 	#else
// 		{
// 		  printf("   | - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - |\n");
// 		  printf("   | Seed not defined - each time test program is run, the test data is the same.|\n");
// 		  printf("   |          To get different test data each time test program is run,          |\n");
// 		  printf("   |           uncomment #define TIME_DRIVEN_RANDOM in rtu_test_main.h           |\n");
// 		  printf("   | - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - |\n");
// 		}
// 		srand(0);
// 	#endif	
// 
// #endif

	t = time(NULL);
	local = localtime(&t);
	printf("Test started (time and date): %s\n", asctime(local));
	
#ifdef CONTINUOUS_TEST	
    
    #ifdef VERBOSE_MODE	
	   err = test_rtu_long_test(1);
    #else
	   err = test_rtu_long_test(0);
    #endif
    
#endif

#ifdef SINGALE_TEST
    //SINGLE MODE
    #ifdef VERBOSE_MODE	
	    err = test_rtu(1);
    #else
	    err = test_rtu(0);
    #endif

#endif

#ifdef MULTI_LOOP_TEST	

    #ifdef VERBOSE_MODE	
	   err = test_rtu_long_test(1);
	   
    #else
	   err = test_rtu_long_test(0);
    #endif
    
#endif

#ifdef SINGLE_LOOP_TEST	

    #ifdef VERBOSE_MODE	
	   err = test_rtu_cont(1);
	   
    #else
	   err = test_rtu_cont(0);
    #endif
    
#endif


	//repeateing tests again and again
	// [does not work yet]
	
// 	for(i = 0; i < iter_number;i++)
// 	{
// 	  iter_error = test_rtu();
// 	  err +=iter_error;
// 	  if(iter_error ==0 )
// 	    printf("\n\n Iteration number: %d without errors\n\n",i);
// 	  else
// 	    printf("\n\n Iteration number: %d with %d errors\n\n",i, abs(iter_error));
// 	}
	
	//just good/bad and how bad (in error number)
	summary(err);//rtu_test.c
	
		t = time(NULL);
	local = localtime(&t);
	printf("Test finished (time and date): %s\n", asctime(local));
	
	return;
	


}
