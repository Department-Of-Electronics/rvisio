# Makefile – FreeRTOS: stack overflow demonstration (RV32IM)
#   make                    overflow check enabled (configCHECK_FOR_STACK_OVERFLOW = 2)
#   make OVERFLOW_CHECK=0   no check – the stack of "deep" overwrites the memory of task "counter"
OVERFLOW_CHECK ?= 2
CROSS  ?= riscv64-unknown-elf-
KERNEL  = FreeRTOS-Kernel
PORT    = port

CFLAGS  = -march=rv32im_zicsr -mabi=ilp32 -Og -g -ffreestanding -nostdlib -Wall \
          -ffunction-sections -fdata-sections \
          -Isrc -Isrc/libc -I$(PORT) -I$(KERNEL)/include \
          -DconfigCHECK_FOR_STACK_OVERFLOW=$(OVERFLOW_CHECK)
ASFLAGS = $(CFLAGS)
LDFLAGS = -T link.ld -nostartfiles -nostdlib -Wl,--no-relax -Wl,--gc-sections -Wl,-Map=build/app.map

SRCS = src/startup.S src/main.c src/libc_min.c \
       $(PORT)/port.c $(PORT)/portASM.S $(PORT)/port_soc.c \
       $(KERNEL)/tasks.c $(KERNEL)/queue.c $(KERNEL)/list.c $(KERNEL)/timers.c \
       $(KERNEL)/event_groups.c $(KERNEL)/stream_buffer.c \
       $(KERNEL)/portable/MemMang/heap_4.c

build/app.elf: $(SRCS) src/*.h $(PORT)/*.h link.ld Makefile build/check-$(OVERFLOW_CHECK)
	mkdir -p build
	$(CROSS)gcc $(CFLAGS) $(LDFLAGS) $(SRCS) -lgcc -o $@
	$(CROSS)objdump -d -S $@ > build/app.lst
	$(CROSS)size $@

# changing OVERFLOW_CHECK rebuilds the program
build/check-$(OVERFLOW_CHECK):
	mkdir -p build
	rm -f build/check-*
	touch $@

clean:
	rm -rf build
