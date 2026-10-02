/* link.ld – FreeRTOS on the simulator's default SoC
 *   FLASH 0x00000000, 64 KB – code (kernel + port + application) and constants
 *   SRAM  0x00010000, 64 KB – variables, heap (configTOTAL_HEAP_SIZE), ISR stack, stack for main
 * Task stacks are in the heap (xTaskCreate); main uses the end of SRAM only until the kernel starts.
 */
OUTPUT_ARCH(riscv)
ENTRY(_start)

MEMORY {
  FLASH (rx)  : ORIGIN = 0x00000000, LENGTH = 64K
  SRAM  (rwx) : ORIGIN = 0x00010000, LENGTH = 64K
}

SECTIONS {
  .text   : { *(.text.start) *(.text*) } > FLASH
  .rodata : { *(.rodata*) *(.srodata*) } > FLASH
  .data   : { *(.data*) *(.sdata*) } > SRAM
  .bss    : {
    __bss_start = .;
    *(.bss*) *(.sbss*) *(COMMON)
    . = ALIGN(4);
    __bss_end = .;
  } > SRAM
  __stack_top = ORIGIN(SRAM) + LENGTH(SRAM);
  ASSERT(__bss_end + 2048 <= __stack_top, "SRAM is full: reduce configTOTAL_HEAP_SIZE")
}
