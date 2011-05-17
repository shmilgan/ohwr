#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <zlib.h>

#define ENV_SIZE 0x4200
#define ENV_OFFSET 0x4200

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

unsigned char buf[ENV_SIZE];

main(int argc, char *argv[])
{
  FILE *f_image, *f_env;
  char line[1024];
  int pos=0;

  if(argc<3)
  {
	printf("usage: uboot_env [flash_image] [environment file]\n");
  }

  f_image = fopen(argv[1],"rb+"); if(!f_image) die ("error opening image file");
  f_env = fopen(argv[2],"r"); if(!f_env) die ("error opening environment file");

  memset(buf, 0, ENV_SIZE);

    while(!feof(f_env))
    {
        fgets(line, 1024, f_env);

        int len = strlen(line);
        if(line[len-1] == '\n')
        {
	    	line[len-1] = 0;
  	    	len--;
  	    }

        memcpy(buf+4+pos, line, len+1);
        pos+=len+1;

    }

	unsigned int crc = crc32(0, buf+4, ENV_SIZE-4);

    fseek(f_image, ENV_OFFSET, SEEK_SET);
	fwrite(&crc, 4, 1, f_image);
    fwrite(buf+4, 1, ENV_SIZE-4, f_image);


    fclose(f_image);
    fclose(f_env);



    return 0;

}
