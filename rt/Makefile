CROSS_COMPILE ?= lm32-elf-

LIBSOFTPLL_DIR = softpll


OBJS = main.o dev/uart.o dev/timer.o lib/mprintf.o dev/ad9516.o ipc/minipc-mem-server.o ipc/rt_ipc.o 

include softpll/softpll.mk

CFLAGS_PLATFORM = -mmultiply-enabled -mbarrel-shift-enabled -Idev
LDFLAGS_PLATFORM = -mmultiply-enabled -mbarrel-shift-enabled   -nostdlib -T target/lm32/ram.ld
OBJS_PLATFORM=target/lm32/crt0.o target/lm32/irq.o

CC=$(CROSS_COMPILE)gcc
OBJCOPY=$(CROSS_COMPILE)objcopy
OBJDUMP=$(CROSS_COMPILE)objdump
CFLAGS= $(CFLAGS_PLATFORM) -ffunction-sections -fdata-sections -O3 -Iinclude -include include/trace.h  -ffreestanding -Iipc -I$(LIBSOFTPLL_DIR) -I.
LDFLAGS= $(LDFLAGS_PLATFORM) -Wl,--gc-sections -O3 -Iinclude -ffreestanding 
SIZE = $(CROSS_COMPILE)size
OBJS += $(OBJS_PLATFORM)
REVISION=$(shell git describe --always --dirty=+ | sed 's/wr-switch-sw-//')

OUTPUT=rt_cpu

all: 		$(OBJS)
				$(SIZE) -t $(OBJS)
				echo "const char *build_revision = \"$(REVISION)\";" > revision.c
				echo "const char *build_date = __DATE__ \" \" __TIME__;" >> revision.c
				$(CC) $(CFLAGS) -c revision.c
				${CC} -o $(OUTPUT).elf $(OBJS) revision.o $(LDFLAGS)
				${OBJCOPY} -O binary $(OUTPUT).elf $(OUTPUT).bin

clean:	
				rm -f $(OBJS) $(OUTPUT).elf $(OUTPUT).bin $(OUTPUT).ram

scp:		all
				scp rt_cpu.bin root@pcbe12132:/tftpboot/rootfs/wr/lib/firmware
				
install:	all
				cp $(OUTPUT).bin ../binaries

%.o:		%.c
				${CC} $(CFLAGS) $(LIB_DIR) -c $^ -o $@

