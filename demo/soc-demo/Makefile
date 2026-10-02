# Makefile – build for RISC-V RV32IM (FemtoRV32 in the simulator)
CROSS  ?= riscv64-unknown-elf-
CFLAGS  = -march=rv32im_zicsr -mabi=ilp32 -O0 -g -ffreestanding -nostdlib -Wall
LDFLAGS = -T link.ld -nostartfiles -Wl,--no-relax

build/app.elf: startup.S main.c link.ld
	mkdir -p build
	$(CROSS)gcc $(CFLAGS) $(LDFLAGS) startup.S main.c -o $@
	$(CROSS)objdump -d $@ > build/app.lst

clean:
	rm -rf build
