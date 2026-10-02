# Makefile, RISC-V (RV32IM) for the SoC simulator
# Linux / WSL:          riscv64-unknown-elf-gcc   (sudo apt install gcc-riscv64-unknown-elf)
# Windows without WSL:  xPack riscv-none-elf-gcc  ->  make CROSS=riscv-none-elf-
CROSS  ?= riscv64-unknown-elf-
CC      = $(CROSS)gcc
CFLAGS  = -march=rv32im_zicsr -mabi=ilp32 -O0 -g -ffreestanding -nostdlib -Wall
LDFLAGS = -T link.ld -nostartfiles -Wl,--no-relax

app.elf: crt0.S main.c soc.h link.ld
	$(CC) $(CFLAGS) $(LDFLAGS) crt0.S main.c -o $@
	$(CROSS)objdump -d $@ > app.lst

clean:
	rm -f app.elf app.lst
