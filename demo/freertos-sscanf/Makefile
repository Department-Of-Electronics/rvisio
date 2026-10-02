# Makefile - FreeRTOS: sscanf() in an RTOS (RV32IM)
#   make DEMO=race                      two tasks, non-thread-safe sscanf -> race condition (default)
#   make DEMO=safe                      the same with a reentrant sscanf -> no errors
#   make DEMO=stack                     sscanf on a small task stack -> stack overflow, caught by the kernel
#   make DEMO=stack OVERFLOW_CHECK=0    the same without the check -> nobody notices the damage
#   make DEMO=crash                     even smaller stack -> the overflow skips the guard and the program crashes
DEMO   ?= race
OVERFLOW_CHECK ?= 2
CROSS  ?= riscv64-unknown-elf-
KERNEL  = FreeRTOS-Kernel
PORT    = port

DEMO_NUM = $(if $(filter safe,$(DEMO)),2,$(if $(filter stack,$(DEMO)),3,$(if $(filter crash,$(DEMO)),4,1)))

CFLAGS  = -march=rv32im_zicsr -mabi=ilp32 -Og -g -ffreestanding -nostdlib -Wall \
          -ffunction-sections -fdata-sections \
          -Isrc -Isrc/libc -I$(PORT) -I$(KERNEL)/include -DDEMO=$(DEMO_NUM) \
          -DconfigCHECK_FOR_STACK_OVERFLOW=$(OVERFLOW_CHECK)
LDFLAGS = -T link.ld -nostartfiles -nostdlib -Wl,--no-relax -Wl,--gc-sections -Wl,-Map=build/app.map

SRCS = src/startup.S src/main.c src/mini_scanf.c src/libc_min.c \
       $(PORT)/port.c $(PORT)/portASM.S $(PORT)/port_soc.c \
       $(KERNEL)/tasks.c $(KERNEL)/queue.c $(KERNEL)/list.c $(KERNEL)/timers.c \
       $(KERNEL)/event_groups.c $(KERNEL)/stream_buffer.c \
       $(KERNEL)/portable/MemMang/heap_4.c

STAMP = build/demo-$(DEMO)-check$(OVERFLOW_CHECK)

build/app.elf: $(SRCS) src/*.h $(PORT)/*.h link.ld Makefile $(STAMP)
	mkdir -p build
	$(CROSS)gcc $(CFLAGS) $(LDFLAGS) $(SRCS) -lgcc -o $@
	$(CROSS)objdump -d -S $@ > build/app.lst
	$(CROSS)size $@

# changing DEMO or OVERFLOW_CHECK rebuilds the program
$(STAMP):
	mkdir -p build
	rm -f build/demo-*
	touch $@

clean:
	rm -rf build
