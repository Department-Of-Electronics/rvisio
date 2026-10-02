/* link.ld – what goes where in the SoC memory (default simulator configuration)
 *   FLASH 0x00000000, 64 KB – code and constants (read-only)
 *   SRAM  0x00010000, 64 KB – variables and stack
 */
OUTPUT_ARCH(riscv)
ENTRY(_start)

MEMORY {
  FLASH (rx)  : ORIGIN = 0x00000000, LENGTH = 64K
  SRAM  (rwx) : ORIGIN = 0x00010000, LENGTH = 64K
}

SECTIONS {
  .text   : { *(.text.start) *(.text*) } > FLASH     /* startup.S must be first, at address 0 */
  .rodata : { *(.rodata*) *(.srodata*) } > FLASH
  .data   : { *(.data*) *(.sdata*) } > SRAM          /* GDB "load" writes .data directly into SRAM */
  .bss    : {
    __bss_start = .;
    *(.bss*) *(.sbss*) *(COMMON)
    . = ALIGN(4);
    __bss_end = .;
  } > SRAM
  __stack_top = ORIGIN(SRAM) + LENGTH(SRAM);         /* the stack grows downward from the end of SRAM */
}
