#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <libgen.h>

#include <inttypes.h>

#include "serial.h"
#include "applet.h"

#define ID_SAM9263 0x019607A2
#define SERIAL_TIMEOUT 100000

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

#define INTERNAL_SRAM_BUF 		0x301000
#define SDRAM_START 			0x20000000


#define PORT_SPEED 			115200

char *program_path;

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
//    printf("XMDump: ");
  //  for(i=0;i<133;i++) printf("%02x ", data[i]);
//    printf("\n\n");
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
	return rval;
}


static int samba_send_file(const char *filename, uint32_t address, uint32_t offset, uint32_t size, int quiet)
{
  FILE *f;
  unsigned char *buf;
  uint32_t file_size, sent;
  int idx = 0;
  char tmp[128];

  uint32_t tstart;

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

  buf = malloc(size);
  if(!buf) die("malloc failed");

  fread(buf, 1, size, f);
  fclose(f);

  snprintf(tmp, 128, "S%08x,%04x#", address, size);
  serial_write(tmp, strlen(tmp));

  tstart = sys_get_clock_usec();

  while(1)
  {
	  if(serial_data_avail())
	  {

		if(serial_read_byte() == 'C') break;
	  }

	  if(sys_get_clock_usec()-tstart>SERIAL_TIMEOUT) die("xmodem handshake timeout");
	}


  file_size = size;
  sent = 0;
  idx=  1;
  while(size > 0)
  {
	uint32_t tosend = size > 128 ? 128 : size;
	char tmp[128];

	memset(tmp, 0, 128);
	memcpy(tmp, buf + sent, tosend);

	sent+=tosend;
	size-=tosend;

  write_xmodem(idx & 0xff, tmp);
//	printf("dix %d \n", idx);

	if(!quiet) printf("%s: %d/%d bytes [%.0f%%] sent.                  \r", filename, sent, file_size, (float)sent/ (float)file_size * 100.0);
	idx++;
  }

  serial_write_byte(0x04);

  free(buf);
  if(!quiet) printf("\n");

  return sent;

}

static void samba_load_applet(char *applet_name, uint32_t address)
{
	char namebuf[1024];

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
		  if( (c=serial_read_byte()) == '>')
			return;
  		  else if(!applet_silent_mode)
			fprintf(stderr,"%c", c);
	  }
	}
}



int samba_connect()
{
    char handshake[] = {0x80, 0x80, 0x23}, cmd[128], buffer[16384];
    int tstart,i,length,npages;
	int c;



    serial_write(handshake,3);
	sys_delay(100);

	if(samba_read(0xffffee40, 4, SERIAL_TIMEOUT) != ID_SAM9263) die ("unknown CPU id");

	samba_load_applet("isp-extram-at91sam9263", INTERNAL_SRAM_BUF);
	mbox_write(INTERNAL_SRAM_BUF, MBOX_COMMAND, APPLET_CMD_INIT);


	mbox_write(INTERNAL_SRAM_BUF, MBOX_TRACELEVEL, 0);
	mbox_write(INTERNAL_SRAM_BUF, MBOX_EXTRAM_RAMTYPE, 0);
	mbox_write(INTERNAL_SRAM_BUF, MBOX_EXTRAM_VDDMEM, 1);
	mbox_write(INTERNAL_SRAM_BUF, MBOX_EXTRAM_BUSWIDTH, 32);

	samba_run(INTERNAL_SRAM_BUF, 100000000);

	if((samba_read(INTERNAL_SRAM_BUF + MBOX_COMMAND, 4, 100000)) != ~APPLET_CMD_INIT) die("invalid response from applet");
	if((samba_read(INTERNAL_SRAM_BUF + MBOX_STATUS, 4, 100000)) != APPLET_SUCCESS) die("invalid response from applet");




}

int dataflash_init()
{

//	samba_write(0xfffff410, 0x04000000, 4, 100000);
//	samba_write(0xfffff430, 0x04000000, 4, 100000);

	samba_load_applet("isp-dataflash-at91sam9263", SDRAM_START);

	mbox_write(SDRAM_START, MBOX_COMMAND, APPLET_CMD_INIT);
	mbox_write(SDRAM_START, MBOX_COMTYPE, 1);
	mbox_write(SDRAM_START, MBOX_TRACELEVEL, 4);
	mbox_write(SDRAM_START, MBOX_DATAFLASH_INDEX, 0);

	samba_run(SDRAM_START, 1000000);

	if(samba_read(SDRAM_START + MBOX_COMMAND, 4, 100000) != ~APPLET_CMD_INIT) die("invalid response from applet");
	if(samba_read(SDRAM_START + MBOX_STATUS, 4, 100000) != APPLET_SUCCESS) die("invalid response from applet");

	buffer_addr = samba_read(SDRAM_START + MBOX_DATAFLASH_BUFFER_ADDR, 4, 100000);

}




int dataflash_write(uint32_t offset, uint32_t buf_addr, uint32_t size)
{

	applet_silent_mode=0;
	printf("Dataflash: Writing %d bytes at offset 0x%x....", size, offset);
    mbox_write(SDRAM_START, MBOX_DATAFLASH_BUF_ADDR, buf_addr);
    mbox_write(SDRAM_START, MBOX_DATAFLASH_BUF_SIZE, size);
    mbox_write(SDRAM_START, MBOX_DATAFLASH_MEM_OFFSET, offset);

	mbox_write(SDRAM_START, MBOX_COMMAND, APPLET_CMD_WRITE);

    samba_run(SDRAM_START, 0);

	if(samba_read(SDRAM_START + MBOX_COMMAND, 4, 100000) != ~APPLET_CMD_WRITE) die("invalid response from applet");
	if(samba_read(SDRAM_START + MBOX_STATUS, 4, 100000) != APPLET_SUCCESS) die("invalid response from applet");


	printf("done. \n\n");


}

int dataflash_erase_all()
{
	mbox_write(INTERNAL_SRAM_BUF, MBOX_COMMAND, APPLET_CMD_FULL_ERASE);
	samba_run(INTERNAL_SRAM_BUF, 0);
}

int dataflash_program(const char *filename)
{

	uint32_t len = samba_send_file(filename, SDRAM_START + 0x3000000, 0, 0, 0);
	dataflash_write(0, SDRAM_START + 0x3000000, len);
}

main(int argc, char *argv[])
{
	char *serial_port = "/dev/ttyS0";


	if(argc < 2)
	{
	  printf("WhiteRabbit MCH DataFlash programmer (c) T.W. 2010\n");
	  printf("Usage: %s <dataflash image> [serial port (default = /dev/ttyS0)]\n", argv[0]);
	  return 0;
	}

	if(argc > 2)
	  serial_port = argv[2];


	program_path = dirname(argv[0]);

	serial_open(serial_port, PORT_SPEED);
	printf("Initializing SAM-BA: ");
	fflush(stdout);

	samba_connect();
	dataflash_init();

	printf("done.\n");
	printf("Programming DataFlash...\n");
	dataflash_program(argv[1]);
	printf("Programming done!\n");

    serial_close();

    return 0;

}