PREFIX  = arm-none-eabi
CC      = $(PREFIX)-gcc
OBJCOPY = $(PREFIX)-objcopy
SIZE    = $(PREFIX)-size

BSP     = vw_nc03/fw/nuc131bsp
DEV     = $(BSP)/Library/Device/Nuvoton/NUC131/Source

ARCH    = -mcpu=cortex-m0 -mthumb -msoft-float
INCS    = -I. -I$(BSP)/Library/CMSIS/Include \
          -I$(BSP)/Library/Device/Nuvoton/NUC131/Include \
          -I$(BSP)/Library/StdDriver/inc
CFLAGS  = $(ARCH) $(INCS) -Os -std=gnu99 -Wall -Wextra \
          -ffunction-sections -fdata-sections -fno-common
LDFLAGS = $(ARCH) -T$(DEV)/GCC/gcc_arm.ld --specs=nano.specs --specs=nosys.specs \
          -Wl,--gc-sections -Wl,-Map=fw.map

OBJS = main.o system_NUC131.o startup_NUC131.o

all: fw.bin

fw.elf: $(OBJS)
	$(CC) $^ $(LDFLAGS) -o $@
	$(SIZE) $@

fw.bin: fw.elf
	$(OBJCOPY) -O binary $< $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

system_NUC131.o: $(DEV)/system_NUC131.c
	$(CC) $(CFLAGS) -c $< -o $@

startup_NUC131.o: $(DEV)/GCC/startup_NUC131.S
	$(CC) $(CFLAGS) -c $< -o $@

flash: fw.bin
	/opt/openocd-nuvoton/bin/openocd -f vw_nc03/fw/nuc131.cfg \
	  -c "init; reset halt; numicro chip_erase; program fw.bin 0x0 verify; reset; exit"

clean:
	rm -f *.o *.elf *.bin *.map

.PHONY: all flash clean
