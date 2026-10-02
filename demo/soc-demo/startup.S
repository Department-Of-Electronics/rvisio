/* startup.S – first instructions after reset (address 0x0):
 * set up the stack and the interrupt handler address (mtvec), clear .bss (uninitialized variables = 0),
 * call main, and when main returns – ebreak halts the processor. */

    .section .text.start
    .globl _start
_start:
    la    sp, __stack_top       # stack at the end of SRAM (from link.ld)

    la    t0, trap_handler      # interrupt handler (main.c): the processor jumps here on an interrupt
    csrw  mtvec, t0

    la    t0, __bss_start       # .bss = 0
    la    t1, __bss_end
1:  bgeu  t0, t1, 2f
    sw    zero, 0(t0)
    addi  t0, t0, 4
    j     1b

2:  call  main                  # a0 = return value of main
    ebreak                      # end of program
3:  j     3b
