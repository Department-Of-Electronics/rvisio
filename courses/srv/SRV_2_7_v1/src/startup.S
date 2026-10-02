/* startup.S – first instructions after reset (address 0x0) for FreeRTOS:
 * stack for main (end of SRAM), mtvec = FreeRTOS port trap handler (portASM.S), .bss = 0, main. */

    .section .text.start
    .globl _start
_start:
    la    sp, __stack_top

    la    t0, freertos_risc_v_trap_handler   # all interrupts go through the FreeRTOS port
    csrw  mtvec, t0

    la    t0, __bss_start
    la    t1, __bss_end
1:  bgeu  t0, t1, 2f
    sw    zero, 0(t0)
    addi  t0, t0, 4
    j     1b

2:  call  main                  # vTaskStartScheduler() does not return
    ebreak
3:  j     3b
