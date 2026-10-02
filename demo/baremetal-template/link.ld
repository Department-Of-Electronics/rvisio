/* link.ld, from the SoC memory map: code and constants in flash, data and stack in sram */
OUTPUT_ARCH(riscv)
ENTRY(_start)
MEMORY {
  ROM (rx)  : ORIGIN = 0x00000000, LENGTH = 0x00010000
  RAM (rwx) : ORIGIN = 0x00010000, LENGTH = 0x00010000
}
SECTIONS {
  .text   : { *(.text.start) *(.text*) } > ROM
  .rodata : { *(.rodata*) *(.srodata*) } > ROM
  /* GDB "load" writes .data directly into RAM (no copying from Flash) */
  .data   : { *(.data*) *(.sdata*) } > RAM
  .bss    : { __bss_start = .; *(.bss*) *(.sbss*) *(COMMON) . = ALIGN(4); __bss_end = .; } > RAM
  __stack_top = ORIGIN(RAM) + LENGTH(RAM);
}
