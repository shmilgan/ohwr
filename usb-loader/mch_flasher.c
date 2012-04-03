/* V3 Flasher - temporary version */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <libgen.h>

#include <inttypes.h>

#include "serial.h"
#include "applet.h"

#define BOARD_REV_V2 2
#define BOARD_REV_V3 3


#define ID_SAM9263 0x019607A2
#define ID_SAM9G45 0x819b05a2
#define SERIAL_TIMEOUT 10000000

#define MBOX_COMMAND 			0x4
#define MBOX_STATUS 			0x8
#define MBOX_COMTYPE			0xc
#define MBOX_TRACELEVEL			0x10
#define MBOX_EXTRAM_VDDMEM		0x14
#define MBOX_EXTRAM_RAMTYPE		0x18
#define MBOX_EXTRAM_BUSWIDTH	0x1c
#define MBOX_EXTRAM_DDRMODEL	0x20
#define MBOX_DATAFLASH_INDEX	0x14
#define MBOX_DATAFLASH_BUFFER_ADDR	0x10
#define MBOX_PROG_BUF_SIZE	0x

#define MBOX_DATAFLASH_BUF_ADDR 0xc
#define MBOX_DATAFLASH_BUF_SIZE 0x10
#define MBOX_DATAFLASH_MEM_OFFSET 0x14

#define INTERNAL_SRAM_BUF 		0x300000
#define SDRAM_START 			0x70000000


#define PORT_SPEED 			115200

//External variable from version.c
extern const char build_time[];
extern const char git_user[];
extern const char git_revision[];

char *program_path;
char *program_name;
int applet_silent_mode = 1;

unsigned int buffer_size = 0x3000;
unsigned int buffer_addr ;

int crc16(int value, int crcin)
{
    int k = (((crcin >> 8) ^ value) & 255) << 8;
    int crc = 0;
    int bits = 8;
    do
    {
        if (( crc ^ k ) & 0x8000)
            crc = (crc << 1) ^ 0x1021;
        else
            crc <<= 1;
        k <<= 1;
    }
    while (--bits);
    return ((crcin << 8) ^ crc);
}


static int write_xmodem(int index, char *p)
{
    unsigned char data[133],c;
    unsigned short crc=0;
    int i;
    data[0]=1;
    data[1]=index;
    data[2]=0xff-index;

    memcpy(data+3,p,128);
    for(i=3;i<131;i++){
     crc=crc16(data[i],crc);
    }
    data[131] = (crc>>8)&0xff;
    data[132] = (crc)&0xff;
    printf("XMDump: ");
    for(i=0;i<133;i++) printf("%02x ", data[i]);
    printf("\n\n");
    serial_write(data,133);
    return serial_read_byte();
}


void die(const char *fmt, ...)
{
	va_list ap;
	
	va_start(ap, fmt);
	fprintf(stderr, "Error: ");
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n\n");
	va_end(ap);
	exit(-1);
}



void samba_write (uint32_t addr, uint32_t data, int size, int timeout)
{
	char tmpbuf[1024],c;
	uint32_t tstart;

	serial_purge();
	snprintf(tmpbuf, 1024, "%c%08x,%08x#", size==1?'O':(size==2?'H':'W'), addr, data);
	serial_write(tmpbuf, strlen(tmpbuf));

    tstart = sys_get_clock_usec();

	while(1)
	{
	  if(serial_data_avail())
	  {		
		if(serial_read_byte() =='>') break;
	  }
	  if(sys_get_clock_usec() - tstart > timeout) die("tx timeout");
	}
  

  

}


uint32_t samba_read (uint32_t addr, int size, int timeout)
{
	char tmpbuf[1024],c=0;
	int i,xpos=-1;
	uint32_t rval;
	uint32_t tstart;
	
	serial_purge();
	snprintf(tmpbuf, 1024, "%c%08x,#", size==1?'o':(size==2?'h':'w'), addr);
	
//	printf("--> cmd: '%s'\n", tmpbuf);

	serial_write(tmpbuf, strlen(tmpbuf));

    tstart = sys_get_clock_usec();

	for(i=0;i<1024 && c!='>';) 
	{ 
	  if(serial_data_avail())
	  {		
		tmpbuf[i]=c=serial_read_byte();
//		fprintf(stderr,"%c", c);
		if(c=='x') xpos=i; 
		i++;
	  }

    if(sys_get_clock_usec()-tstart>timeout) die("rx timeout");
	  
	}
	
	if(xpos<0) die("invalid response from samba");
	
	sscanf(tmpbuf, "%x", &rval);
//	fprintf(stderr,"samba_read: %x\n", rval);
	return rval;	
}


static int samba_send_file(const char *filename, uint32_t address, uint32_t offset, uint32_t size, int quiet)
{
  FILE *f;
  unsigned char *buf;
  uint32_t file_size, sent;
  int idx = 0;
  int boffset = 0;
  char tmp[4097];

  uint32_t tstart;
    
//    printf("SendFile: %s\n", filename);
    
  f = fopen(filename, "rb");
  if(!f) die("file open error: '%s'",filename);
  
  
  if(!size)
  {
	fseek(f, 0, SEEK_END);
    size = ftell(f);
    offset = 0;
	rewind(f);
  } else 
	fseek(f, offset, SEEK_SET);

  buf = malloc(size+1);
  if(!buf) die("malloc failed");
  
  fread(buf, 1, size, f);
  fclose(f);

//    printf("Send: %s\n", filename);

    while(size > 0)
    {
      int tosend = size > 1024 ? 1024 : size;
//      printf("Sending %d bytes\n", tosend);

      snprintf(tmp, 128, "S%08x,%x#", address, tosend);
      serial_write(tmp, strlen(tmp));
      memcpy(tmp, buf + boffset, tosend);
      tmp[tosend]='\n';
      serial_write(tmp, tosend+1);
      
      size -= tosend;
      boffset +=tosend;
      address += tosend;

      while(1)
      {	
        if(serial_data_avail()) 
	    if(serial_read_byte() == '>')
		break;
      }
    }

  free(buf);

  return boffset;

}

static void samba_load_applet(char *applet_name, uint32_t address)
{
	char namebuf[1024];

	printf("loading applet %s at 0x%08x\n", applet_name, address);
	
	snprintf(namebuf, 1024, "%s/samba_applets/%s.bin", program_path, applet_name);
	samba_send_file(namebuf, address, 0, 0, 1);
}


static void mbox_write(uint32_t base, uint32_t offset, uint32_t value)
{
  samba_write(base+offset, value, 4, SERIAL_TIMEOUT);
}	

void samba_run(uint32_t addr, int timeout)
{
	char tmpbuf[1024],c;
	uint32_t tstart,t1;

	serial_purge();
	snprintf(tmpbuf, 1024, "G%08x#",  addr);

	serial_write(tmpbuf, strlen(tmpbuf));
	
	t1 = tstart = sys_get_clock_usec();
	
	while((timeout && (sys_get_clock_usec()- tstart < timeout)) || !timeout)
	{
	  if(serial_data_avail()) {
		c = serial_read_byte();
		if(c == '>') break;
	    if(!applet_silent_mode )fprintf(stderr,"%c", c);
	    t1 = sys_get_clock_usec();
  	  } else if(sys_get_clock_usec() - t1 > 10000)
	  {
	//	fprintf(stderr,"Phase1\n");

//		serial_write_byte('\n');
		serial_write_byte(0x0a);
		serial_write_byte(0x0d);
		serial_write_byte('T');
		serial_write_byte('#');
		serial_write_byte(0x0a);
		serial_write_byte(0x0d);
		usleep(10000);
		while(serial_data_avail())
		{
		  c=serial_read_byte();
		  if( c == '>') 
    		    return; 
  		  else if(!applet_silent_mode)
			fprintf(stderr,"%c", c);
		}
	  }
	}
}



int samba_connect(int board_rev)
{
    char handshake[] = {0x80, 0x80, 0x23}, cmd[128], buffer[16384];
    int tstart,i,length,npages;
	int c;

	serial_write(handshake,3);
	sys_delay(100);

	uint32_t id = samba_read(0xffffee40, 4, SERIAL_TIMEOUT);
	
	printf("CPU ID: 0x%08x\n",id);

	if(board_rev == BOARD_REV_V2 && id != ID_SAM9263) die ("Not a 9263 CPU");
	if(board_rev == BOARD_REV_V3 && id != ID_SAM9G45) die ("Not a 9G45 CPU");
}

int ddr_init(int board_rev)
{
	if(board_rev == BOARD_REV_V2)
		samba_load_applet("isp-extram-at91sam9263", INTERNAL_SRAM_BUF);
	else if(board_rev == BOARD_REV_V3) 
		samba_load_applet("isp-extram-at91sam9g45", INTERNAL_SRAM_BUF); 
	
	mbox_write(INTERNAL_SRAM_BUF, MBOX_COMMAND, APPLET_CMD_INIT);
	
	mbox_write(INTERNAL_SRAM_BUF, MBOX_TRACELEVEL, 0);
	mbox_write(INTERNAL_SRAM_BUF, MBOX_EXTRAM_RAMTYPE, 1);
	mbox_write(INTERNAL_SRAM_BUF, MBOX_EXTRAM_VDDMEM, 0);
	mbox_write(INTERNAL_SRAM_BUF, MBOX_EXTRAM_BUSWIDTH, 16);
	mbox_write(INTERNAL_SRAM_BUF, MBOX_EXTRAM_DDRMODEL, 0);
	
	samba_run(INTERNAL_SRAM_BUF, 100000000);
	
	if((samba_read(INTERNAL_SRAM_BUF + MBOX_COMMAND, 4, 10000000)) != ~APPLET_CMD_INIT) die("invalid response from applet init");
	if((samba_read(INTERNAL_SRAM_BUF + MBOX_STATUS, 4, 10000000)) != APPLET_SUCCESS) die("invalid response from applet status");
	return 0;
}

int ddr_check(int board_rev)
{
	
	mbox_write(INTERNAL_SRAM_BUF, MBOX_COMMAND, APPLET_CMD_WRITE);
	samba_run(INTERNAL_SRAM_BUF, 0);
	if((samba_read(INTERNAL_SRAM_BUF + MBOX_COMMAND, 4, 10000000)) != ~APPLET_CMD_WRITE) die("invalid response from applet write");
	if((samba_read(INTERNAL_SRAM_BUF + MBOX_STATUS, 4, 10000000)) != APPLET_SUCCESS) die("invalid response from applet status");	
}

int dataflash_init(int board_rev)
{

//	samba_write(0xfffff410, 0x04000000, 4, 100000);
//	samba_write(0xfffff430, 0x04000000, 4, 100000);
	
	if(board_rev == BOARD_REV_V2)
		samba_load_applet("isp-dataflash-at91sam9263", INTERNAL_SRAM_BUF);
	else if(board_rev == BOARD_REV_V3)
		samba_load_applet("isp-dataflash-at91sam9g45", INTERNAL_SRAM_BUF);
//	samba_load_applet("isp-dataflash-at91sam9263", SDRAM_START);

	mbox_write(INTERNAL_SRAM_BUF, MBOX_COMMAND, APPLET_CMD_INIT);
	mbox_write(INTERNAL_SRAM_BUF, MBOX_COMTYPE, 1);
	mbox_write(INTERNAL_SRAM_BUF, MBOX_TRACELEVEL, 4);
	mbox_write(INTERNAL_SRAM_BUF, MBOX_DATAFLASH_INDEX, 0);

	samba_run(INTERNAL_SRAM_BUF, 0);

	if(samba_read(INTERNAL_SRAM_BUF + MBOX_COMMAND, 4, 10000000) != ~APPLET_CMD_INIT) die("invalid response from applet");
	if(samba_read(INTERNAL_SRAM_BUF + MBOX_STATUS, 4, 10000000) != APPLET_SUCCESS) die(" DataFlash initialization failure (no chip deteted?)");

	buffer_addr = samba_read(INTERNAL_SRAM_BUF + MBOX_DATAFLASH_BUFFER_ADDR, 4, 10000000);
//	fprintf(stderr,"DF Initialized");
	return 0;
}




int dataflash_write(uint32_t offset, uint32_t buf_addr, uint32_t size)
{

    applet_silent_mode=0;
    fprintf(stderr, "Dataflash: Writing %d bytes at offset 0x%x buffer %x....", size, offset, buf_addr);
    mbox_write(INTERNAL_SRAM_BUF, MBOX_DATAFLASH_BUF_ADDR, buf_addr);
    fprintf(stderr,"A");
    mbox_write(INTERNAL_SRAM_BUF, MBOX_DATAFLASH_BUF_ADDR, buf_addr);
    fprintf(stderr,"B");
    mbox_write(INTERNAL_SRAM_BUF, MBOX_DATAFLASH_BUF_SIZE, size);
    fprintf(stderr,"C");
    mbox_write(INTERNAL_SRAM_BUF, MBOX_DATAFLASH_MEM_OFFSET, offset);
    fprintf(stderr,"D");

    mbox_write(INTERNAL_SRAM_BUF, MBOX_COMMAND, APPLET_CMD_WRITE);
    fprintf(stderr,"E");
	
    samba_run(INTERNAL_SRAM_BUF, 0);
    fprintf(stderr,"F");

	if(samba_read(INTERNAL_SRAM_BUF + MBOX_COMMAND, 4, 10000000) != ~APPLET_CMD_WRITE) die(" invalid response from applet");
	if(samba_read(INTERNAL_SRAM_BUF + MBOX_STATUS, 4, 10000000) != APPLET_SUCCESS) die(" write failure");


	printf("done. \n\n");

  
}

int dataflash_erase_all()
{
	mbox_write(INTERNAL_SRAM_BUF, MBOX_COMMAND, APPLET_CMD_FULL_ERASE);
	samba_run(INTERNAL_SRAM_BUF, 0);
}

int dataflash_program(const char *filename)
{
	uint32_t len = samba_send_file(filename, SDRAM_START, 0, 0, 0);
	dataflash_write(0, SDRAM_START, len);
}


void show_help(const char* serial_port)
{
	printf("WhiteRabbit MCH DataFlash programmer (c) T.W. 2010\n");
	printf("Compiled by %s (%s)\ngit rev:%s\n\n",git_user,build_time,git_revision);
	printf("Usage: %s [options] <dataflash image>\n", program_name);
	printf("Options:\n");
	printf("\t-a \t\t perform all actions (same as -wec). Default option.\n");
	printf("\t-w \t\t write the dataflash\n");
	printf("\t-e \t\t erase the dataflash\n");
	printf("\t-c \t\t check the DDR\n");
	printf("\t-s SERIAL_PORT\t By default it is: -s %s\n",serial_port);
	printf("\t-h \t\t Show this little help\n");
	printf("\n");
}

main(int argc, char *argv[])
{
	int board_rev  = BOARD_REV_V3;
	char *serial_port = "/dev/ttyACM0";
	char c;
	int erase=0;
	int check=0;
	int write=0;
	int noopts=1;
	
	program_name = basename(argv[0]);
	program_path = dirname(argv[0]);		

	//Parse options
	
	while (--argc > 0 && (*++argv)[0] == '-') {
		noopts=0;
		while (c = *++argv[0]) {
			switch (c) {
				case 'a': erase = 1; check=1; write=1; break;
				case 'e': erase=1; break;
				case 'c': check=1; break;
				case 'w': write=1; break;
				case 's': serial_port = argv[1]; break;
				case 'h': show_help(serial_port);
					return 0;
					break;
				default:
					printf("find: illegal option %c\n", c);
					argc = 0;
					break;
			}
		}
	}
	--argv;
	++argc;
		
	//Default value are all
	if(noopts) { erase = 1; check=1; write=1; }
	
	printf("prog=%s, n=%d, serial=%s, (e=%d,c=%d,w=%d)\n", argv[1],argc,serial_port, erase,check,write);
	
	if(write && argc < 2)
	{
		show_help(serial_port);
		return 0;
	}
	
	//Print line to know the version of software for testing purpose
	fprintf(stderr,"\nCompiled by %s (%s)\ngit rev:%s\n\n",git_user,build_time,git_revision);
	
	serial_open(serial_port, PORT_SPEED);
	fprintf(stderr,"Initializing SAM-BA: ");
	samba_connect(board_rev);

	fprintf(stderr,"Initializing DDR...\n\n");
	ddr_init(board_rev);
	
	if(check) 
	{
		fprintf(stderr,"Checking DDR...\n\n");
		ddr_check(board_rev);
	}
	
	fprintf(stderr,"Initializing DataFlash...\n\n");
	dataflash_init(board_rev);

	if(erase) 
	{
		fprintf(stderr,"Erasing DataFlash...\n\n");
		dataflash_erase_all();
	}
	
	if(write)
	{
		fprintf(stderr,"Programming DataFlash...\n");
		dataflash_program(argv[1]);
	}
	
	printf("Programming done!\n");

	serial_close();
    
  return 0;
}
