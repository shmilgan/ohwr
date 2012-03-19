/*
TODO:
- upgrade to newest version of main (with versioning)
- merge constarings, do constraings properly
- fix two know bugs which cause ~20% of tests to fail
- implement normal and continous work of RTU, which means: 
    1) write requetrs/mems 
    2) read responses/ufifo
    3) write more requests mems
    4) switch banks
    5) read more requets/ufifos
    5) go to 1)
- prepare some good solution when the CAM is full (due
  to error, the fill_in_data function considered CAM
  to be full and did not fill it - this caused error)
- 

*/



/****************************************
    decide on data kind
****************************************/  
/*
  uncomment to use a predefined
  data, only very few examples
*/
//#define	PREDEFINED_DATA

/*
  uncomment to have random data
  filled in htab, vlan, hcam 
  and have requets generated
  randomly
*/
#define RANDOM_DATA

/*
  uncomment to have settings
  generated randomly
*/
#define RANDOM_SETTINGS

/*
  uncomment to base the seed of rand() 
  function on the time - each time
  test app is run, data is different
  if commented, always the same random
  data is generated
*/
#define TIME_DRIVEN_RANDOM

/*
  In order to test that CAM works fine, 
  in case that the randome data does not
  produce CAM entries, it is possible to 
  enforce some entires to be "CAM entries"
  this is done by taking few percent of
  request MACs from CAM (rtu_test_data.c).
  
  if the define is commented, it's not very
  likely there are meny (if any) CAM entires
  since the MACs are taken from the htab entires

*/
#define ENFORCE_DATA_ENTRIES


/****************************************
    define hw/sim run mode
****************************************/  
/*
  if commented, the "flooding mode""
  is on 
*/
//#define HW_TEST_ONE_BY_ONE

/****************************************
    test mode (choose one define only)
****************************************/ 

/*
  mode of operation: single test
  it runs only one full test which includes:
  - clean memories (htab, vlan)
  - fill in memories(htab, vlan)
  - set settings
  - create requests (based on data in mems)
  - run hw/sim
  - compare results
  - output results (errors if occure)
*/

//#define SINGALE_TEST

/*
  mode of operation: single loop test
  it runs TEST_ITERATIONS times hw/sim run without 
  cleaing mems:
  - clean memories
  - fill in memories
  - repeat:
    
    * fill in new settings
    * create requests (based on data in mems)
    * run hw/sim
    * compare results
    * add more entries to htab
    
  - output results (errors if occure)
*/

//#define SINGLE_LOOP_TEST


/*
  mode of operation: multi loop test
  it runs :
  MULTI_LOOP_TEST_NUMBER times loop with 
  TEST_ITERATIONS  times hw/sim run without 
  cleaing mems:
  
  
  - clean memories
  - fill in memories
  - repeat (MULTI_LOOP_TEST_NUMBER times):
  
	-> repeat (TEST_ITERATIONS times):
	
	      * fill in new settings
	      * create requests (based on data in mems)
	      * run hw/sim
	      * compare results
	      * add more entries to htab
	  
	 -> clean aging memories
	 -> clean tab storing hashes of filled in htab entries
	 -> fill in vlan
	 -> fill in htab
    
  - output results (errors if occure)
*/

#define MULTI_LOOP_TEST
//#define CONTINUOUS_TEST


/****************************************
    decide on hash polynomial
****************************************/    
/*
     DECT
*/
//#define USE_POLYNOMIAL  0x0589  
/*
    CCITT
*/    
#define USE_POLYNOMIAL  0x1021 
/*
     IBM
*/     
//#define USE_POLYNOMIAL  0x8005 


/****************************************
    test parameters
****************************************/ 



#define ACTIVE_BANK 1
/*
  Sets how many requests will be send to RTU
  during the test.
  Due to limited learning queue (UFIFO.size=128),
  setting more than 100 requests may result in 
  UFIFO being filled and errors will be reported.
  It is suggested not to set more than 100 reqs
*/
#define TEST_REQS 100
/*
  Number of ports depends on the FPGA settings
*/
#define PORT_NUMBER 8

/*
  Number of test iterations per
  application run during continous mode
*/
#define TEST_ITERATIONS 20

#define MULTI_LOOP_TEST_NUMBER 1000000

/*
  for "#define CONTINUOUS_MODE"
  Not finished yet
*/
#define TEST_ITERATIONS_PER_LOOP 3

/*
  Enables to set custom SEED.
  It is usefull if an error is report 
  during a test. in such case the seed
  with which the test was run, should be 
  noted (it is printed at the beginnig 
  of the test.
  To repeat the test to fix the bug,
  the appropriate seed should be defined 
  here.
*/
//#define CUSTOME_SEED 496
//#define CUSTOME_SEED 176

/****************************************
If we want the test app to start talking
here are a few options for that
****************************************/
//#define VERBOSE_MODE
//#define DEBUG_HTAB
//#define DEBUG_VLAN
//#define DEBUG_REQUEST
//#define DEBUG_RESULTS  
//#define DEBUG_HCAM  
//#define DEBUG_SIMULATION
//#define DEBUG_READING_RESPONSES
//#define ML_DBG

/****************************************
      bug fixing playground
  (restricted area for civils)
****************************************/ 

//resolved
//#define DST_HAS_PRIO_PROBLEM

//resolved
//#define AGING_PROBLEM

// not resolved yet :(
//#define CAM_DROP_PROBLEM
//
//#define CAM_DROP_PROBLEM_2
/*

out of 14 trials in 3 errors occured

seeds that cause problems:

---- this seeds work with #define RANDOM_SETTINGS commented

seems to be algorith issue:
seed:176,
16

228


ok seeds:
1) seed = 6685

repeating tests:

while [ 1 ]; do ./build.sh run T=192.168.1.5; done  > test_results


*/