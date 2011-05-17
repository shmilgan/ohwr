include ../../../../Makedefs

CROSS_COMPILE = $(CROSS_COMPILE_ARM)

OBJS = main.o mi2c.o spi_slave.o
OUTPUT = wd

BOARD_OBJS = ./board/board_cstartup.o ./board/board_lowlevel.o ./board/board_memories.o
LIB_OBJS = ./lib/adc.o ./lib/pio.o ./lib/aic.o ./lib/dbgu.o ./lib/stdio.o ./lib/memcpy.o  ./lib/twi.o ./lib/pmc.o

TRACE_LEVEL = 3  
CHIP = at91sam7xc512
BOARD = whiterabbit_mch
ARCH = arm7tdmi 


CFLAGS = -mcpu=$(ARCH) -Os -mlong-calls -ffunction-sections -D$(CHIP) -DTRACE_LEVEL=$(TRACE_LEVEL) -Iinclude
LDFLAGS = -mcpu=$(ARCH) -nostdlib -lgcc -Wl,--gc-sections -T board/flash.lds

#CROSS_COMPILE = arm-linux-uclibcgnueabi-

ALL_OBJS =   $(BOARD_OBJS) $(LIB_OBJS) $(OBJS)

CC = $(CROSS_COMPILE)gcc
SIZE = $(CROSS_COMPILE)size
STRIP = $(CROSS_COMPILE)strip
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump


all:		$(ALL_OBJS)
				${CC} -o $(OUTPUT).elf $(ALL_OBJS) $(LDFLAGS)
				${OBJDUMP} -d $(OUTPUT).elf > $(OUTPUT)_disasm.S
				${OBJCOPY} -O binary $(OUTPUT).elf $(OUTPUT).bin

%.o:		%.S
				${CC} $(CFLAGS) -D__ASSEMBLY__ -c $^ -o $@

%.o:		%.c
				${CC} $(CFLAGS) -c $^ -o $@

clean:	
				rm -f $(ALL_OBJS) $(OUTPUT).bin $(OUTPUT).elf

flash:		all
				../../../../scripts/flash_watchdog.sh $(OUTPUT).bin
#				com /dev/ttyUSB0 115200