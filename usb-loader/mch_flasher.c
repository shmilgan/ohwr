/* V3 Flasher - temporary version */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>

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

#define MBOX_MEM_BUF_ADDR 0xc
#define MBOX_MEM_BUF_SIZE 0x10
#define MBOX_MEM_BUF_OFFSET 0x14

#define MBOX_MEM_ERASE_TYPE 0xc


#define INTERNAL_SRAM_BUF 		0x300000
#define SDRAM_START 			0x70000000


#define PORT_SPEED 			115200

#define MEMTYPE_DDR 	0
#define MEMTYPE_DF 		1
#define MEMTYPE_NAND 	2

const char* memName[3] = { "DDR", "DataFlash", "NandFlash" };


typedef struct {
	char *fname;
	uint32_t offset;
} sndfile;

sndfile filearray[3];


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

/**
 * filename: name of the file in the host
 * address: place to send to file on the device
 * offset: internal offset, can be use with size
 * size: can be 0
 */
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

	if(!quiet) printf("size 0x%x (%dKb)\n",size,size/1024);

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
		}
		else if(sys_get_clock_usec() - t1 > 10000)
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
		usleep(1000);
		//TODO: Improve data available for DDR load
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
	return 0;
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



int ddr_load(int nFile, const sndfile* pFileArray)
{
	int i;
	uint32_t len=0;
	fprintf(stderr,"Loading DDR...\n");
	for(i=0;i<nFile; i++)
	{
		printf("\t @ 0x%08x : %s ; ",SDRAM_START+ pFileArray[i].offset, pFileArray[i].fname);
		len += samba_send_file(pFileArray[i].fname, SDRAM_START+pFileArray[i].offset,0, 0, 0);
	}
	mem_write(MEMTYPE_DDR,0,SDRAM_START,len);
	fprintf(stderr,"\n");
	return 0;
}



int memflash_init(int type, int board_rev)
{
	fprintf(stderr,"Initializing %s...\n",memName[type]);

	//Load the correct applet
	if(board_rev == BOARD_REV_V2)
	{
		if(type==MEMTYPE_NAND) samba_load_applet("isp-nandflash-at91sam9263", INTERNAL_SRAM_BUF);
		else if(type==MEMTYPE_DF) samba_load_applet("isp-dataflash-at91sam9263", INTERNAL_SRAM_BUF);
	}
	else if(board_rev == BOARD_REV_V3)
	{
		if(type==MEMTYPE_NAND) samba_load_applet("isp-nandflash-at91sam9g45", INTERNAL_SRAM_BUF);
		else if(type==MEMTYPE_DF) samba_load_applet("isp-dataflash-at91sam9g45", INTERNAL_SRAM_BUF);
	}


	mbox_write(INTERNAL_SRAM_BUF, MBOX_COMMAND, APPLET_CMD_INIT);
	mbox_write(INTERNAL_SRAM_BUF, MBOX_COMTYPE, 1);
	mbox_write(INTERNAL_SRAM_BUF, MBOX_TRACELEVEL, 4);
	mbox_write(INTERNAL_SRAM_BUF, MBOX_DATAFLASH_INDEX, 0);

	samba_run(INTERNAL_SRAM_BUF, 0);

	if(samba_read(INTERNAL_SRAM_BUF + MBOX_COMMAND, 4, 10000000) != ~APPLET_CMD_INIT) die("invalid response from applet");
	if(samba_read(INTERNAL_SRAM_BUF + MBOX_STATUS, 4, 10000000) != APPLET_SUCCESS) die("Initialization failure (no chip deteted?)");

	buffer_addr = samba_read(INTERNAL_SRAM_BUF + MBOX_DATAFLASH_BUFFER_ADDR, 4, 10000000);
	fprintf(stderr,"Initializing %s > Done! \n\n",memName[type]);

	return 0;
}



int mem_program(int type, int nFile, const sndfile* pFileArray)
{
	int i;
	uint32_t len=0;
	if(type==MEMTYPE_DDR) return ddr_load(nFile,pFileArray);
	else
	{

		fprintf(stderr,"Programming %s...\n",memName[type]);
		for(i=0;i<nFile; i++)
		{
			printf("\t @ 0x%08x : %s ; ",SDRAM_START+ pFileArray[i].offset, pFileArray[i].fname);
			len = samba_send_file(pFileArray[i].fname, SDRAM_START,0,0,0);
			mem_write(type,pFileArray[i].offset, SDRAM_START, len);
		}
		fprintf(stderr,"Programming %s Done!!!\n",memName[type]);
		return 0;
	}
}

void mem_check(int type)
{
	fprintf(stderr,"Checking %s...\n\n",memName[type]);

	mbox_write(INTERNAL_SRAM_BUF, MBOX_COMMAND, APPLET_CMD_LIST_BAD_BLOCKS);
	samba_run(INTERNAL_SRAM_BUF, 0);
	if((samba_read(INTERNAL_SRAM_BUF + MBOX_COMMAND, 4, 10000000)) != ~APPLET_CMD_LIST_BAD_BLOCKS) die("invalid response from applet write");
	if((samba_read(INTERNAL_SRAM_BUF + MBOX_STATUS, 4, 10000000)) != APPLET_SUCCESS) die("invalid response from applet status");
}

void mem_full_erase(int type)
{
	fprintf(stderr,"Erasing %s...",memName[type]);

	mbox_write(INTERNAL_SRAM_BUF, MBOX_COMMAND, APPLET_CMD_FULL_ERASE);
	samba_run(INTERNAL_SRAM_BUF, 0);

	fprintf(stderr,"\rErasing %s > DONE\n\n",memName[type]);
}


int mem_write(int type, uint32_t offset, uint32_t buf_addr, uint32_t size)
{
	uint32_t timeout;
	applet_silent_mode=1;
	fprintf(stderr, "%s: Writing %d bytes at offset 0x%x buffer %x....", memName[type], size, offset, buf_addr);
	mbox_write(INTERNAL_SRAM_BUF, MBOX_MEM_BUF_ADDR, buf_addr);
	fprintf(stderr,"A");
	mbox_write(INTERNAL_SRAM_BUF, MBOX_MEM_BUF_ADDR, buf_addr);
	fprintf(stderr,"B");
	mbox_write(INTERNAL_SRAM_BUF, MBOX_MEM_BUF_SIZE, size);
	fprintf(stderr,"C");
	mbox_write(INTERNAL_SRAM_BUF, MBOX_MEM_BUF_OFFSET, offset);
	fprintf(stderr,"D");

	mbox_write(INTERNAL_SRAM_BUF, MBOX_COMMAND, APPLET_CMD_WRITE);
	fprintf(stderr,"E");

	if(type==MEMTYPE_DDR) timeout=10;
	else timeout=0;

	samba_run(INTERNAL_SRAM_BUF, timeout);
	fprintf(stderr,"F");

	if(type == MEMTYPE_DDR) // booting a barebox/kernel will fuck up USB, so we'll never get any response
		return 0;

	if(samba_read(INTERNAL_SRAM_BUF + MBOX_COMMAND, 4, 10000000) != ~APPLET_CMD_WRITE) die(" invalid response from applet");
	if(samba_read(INTERNAL_SRAM_BUF + MBOX_STATUS, 4, 10000000) != APPLET_SUCCESS) die(" write failure");

	printf(" OK\n");
}

void nand_scrub()
{
	fprintf(stderr,"Scrubing NAND...");

	mbox_write(INTERNAL_SRAM_BUF, MBOX_COMMAND, APPLET_CMD_FULL_ERASE);
	mbox_write(INTERNAL_SRAM_BUF, MBOX_MEM_ERASE_TYPE,0x0000EA11);
	samba_run(INTERNAL_SRAM_BUF, 0);

	fprintf(stderr,"Done\n\n");
}



uint32_t getOffset(const char *str)
{
	uint32_t ret=0;
	if (str == NULL || *str == '\0') return ret;
	if(str[0]=='0' && (str[1]=='X' || str[1]=='x'))
	{
		str+=2;
		sscanf(str, "%x", &ret);
	}
	else sscanf(str, "%d", &ret);
	return ret;
}


void show_help(const char* serial_port)
{
	printf("WhiteRabbit MCH DataFlash programmer (c) T.W. 2010\n");
	printf("Compiled by %s (%s)\ngit rev:%s\n\n",git_user,build_time,git_revision);
	printf("Usage: %s [options] [<FILE1> <OFFSET1> <FILE2> <OFFSET2> <FILE3> <OFFSET3>]\n", program_name);
	printf("if FILEx set it will write the FILEx at the given OFFSETx (by default 0x0), "
			"otherwise you can use this binary to erase, check or scrub a memory\n");
	printf("Options:\n");
	printf("\t-m \t Select the memory type: ddr, nf, df\n");
	printf("\t-e \t\t erase the memory\n");
	printf("\t-c \t\t check the memory (not available for df)\n");
	printf("\t-s \t\t scrub the memory (only available for nf)\n");
	printf("\t-r \t\t addr run the image at address addr\n");
	printf("\t-p SERIAL_PORT\t By default it is: -p %s\n",serial_port);
	printf("\t-h \t\t Show this little help\n");
	printf("\n");
}

main(int argc, char *argv[])
{
	int board_rev  = BOARD_REV_V3;
	char *serial_port = "/dev/ttyACM0";
	char * mode_str ="df";
	char opt;
	int erase=0, check=0, scrub=0;
	int type=-1;
	unsigned int offset=0, run_addr=0;
	int noopts=1;
	int nFile=0;
	int run =0 ;


	program_name = basename(argv[0]);
	program_path = dirname(argv[0]);

	if(argc==1)
	{
		show_help(serial_port);
		return 1;
	}

	//Parse options
	while ((opt = getopt (argc, argv, "m:p:r:ecsh")) != -1)
		switch (opt) {
		case 'm':
			mode_str = optarg;
			break;
		case 'p':
			serial_port = optarg;
			break;
		case 'e':
			erase = 1;
			break;
		case 'c':
			check = 1;
			break;
		case 's':
			scrub = 1;
			break;
		case 'r':
			run = 1;
			sscanf(optarg,"%i", &run_addr);
			break;

		case 'h':
		case '?':
			show_help(serial_port);
			return 0;
		default:
			printf("find: illegal option %c\n", opt);
			break;
		}



	//Obtain the various filename & offset (3 MAX)
	for(nFile=0; optind < argc; nFile++)
	{
		filearray[nFile].fname=argv[optind++];
		filearray[nFile].offset=getOffset(argv[optind++]);
	}

	printf("n=%d, mode=%s, nFile=%d, serial=%s, (e=%d,c=%d,s=%d)\n", argc, mode_str, nFile, serial_port, erase,check,scrub);

	if (serial_open(serial_port, PORT_SPEED) < 0) {
		fprintf(stderr, "%s: %s: %s\n", argv[0], serial_port,
			strerror(errno));
		exit(1);
	}
	fprintf(stderr,"Initializing SAM-BA: ");
	samba_connect(board_rev);

	//Init the DDR for every action (used as buffer)
	fprintf(stderr,"\nInitializing DDR...\n");
	ddr_init(board_rev);
	fprintf(stderr,"Initializing DDR > Done\n\n");

	//Obtain the memory type mode
	if(strcmp(mode_str,"nand")==0 || strcmp(mode_str,"nf")==0) {
		type=MEMTYPE_NAND;
	} else if(strcmp(mode_str,"dataflash")==0 || strcmp(mode_str,"df")==0) {
		type=MEMTYPE_DF;
	} else if(strcmp(mode_str,"ddr")==0 || strcmp(mode_str,"ram")==0) {
		type=MEMTYPE_DDR;
	} else {
		fprintf(stderr,"Bad memory type %s...\n\n",mode_str);
		show_help(serial_port);
		return 0;
	}


	//Init the memory (except DDR which is already init)
	if(type!=MEMTYPE_DDR)
		memflash_init(type, board_rev);

	//Run the action
	if(erase)
		mem_full_erase(type);
	if(check)
		mem_check(type);
	if(scrub && type==MEMTYPE_NAND)
		nand_scrub();
	if(nFile>0)
		mem_program(type,nFile,(const sndfile*)&filearray);

	if(run)
	{
		fprintf(stderr,"Executing payload @ %x\n", run_addr);
		samba_run(run_addr, 0);
	}

	serial_close();
	printf("Closing...\n");

	return 0;
}
