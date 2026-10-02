# Makefile – example of the course "Real Time Operating Systems" on the RVisio SoC, RV32IM
CROSS  ?= riscv64-unknown-elf-
KERNEL  = FreeRTOS-Kernel
PORT    = port
HAL     = hal

CFLAGS  = -march=rv32im_zicsr -mabi=ilp32 -Og -g -ffreestanding -nostdlib -Wall \
          -ffunction-sections -fdata-sections \
          -Isrc -Isrc/libc -I$(HAL) -I$(PORT) -I$(KERNEL)/include
ASFLAGS = $(CFLAGS)
LDFLAGS = -T link.ld -nostartfiles -nostdlib -Wl,--no-relax -Wl,--gc-sections -Wl,-Map=build/app.map

SRCS = src/startup.S src/main.c src/util.c src/libc_min.c \
       $(HAL)/hal_led.c $(HAL)/hal_7seg.c \
       $(PORT)/port.c $(PORT)/portASM.S $(PORT)/port_soc.c \
       $(KERNEL)/tasks.c $(KERNEL)/queue.c $(KERNEL)/list.c $(KERNEL)/timers.c $(KERNEL)/event_groups.c \
       $(KERNEL)/portable/MemMang/heap_4.c

build/app.elf: $(SRCS) src/*.h $(HAL)/*.h $(PORT)/*.h link.ld Makefile
	mkdir -p build
	$(CROSS)gcc $(CFLAGS) $(LDFLAGS) $(SRCS) -lgcc -o $@
	$(CROSS)objdump -d -S $@ > build/app.lst
	$(CROSS)size $@

clean:
	rm -rf build
