// main.c – interrupts on the SoC: timer and UART receive via the interrupt controller (INTC).
//
// The main program "does work" (counts in a loop), and interrupts interrupt it:
//   * timer every 2000 cycles  -> the handler increments ticks and shows it on the LEDs
//   * UART receives a character (type it in the "Console" panel) -> the handler echoes it in upper case
// After 20 timer interrupts the program prints the result and finishes.
//
// Addresses and interrupt numbers come from the default SoC configuration ("Memory map" panel).
// Try it in VS Code: a breakpoint in trap_handler (e.g. on ticks++), F5 – then in the browser look at
// the Processor panel (mepc, mcause) and the Stack analyzer (the handler saves registers on the stack).
#include <stdint.h>

#define REG(a) (*(volatile uint32_t *)(a))

// UART0 (interrupt 0)
#define UART_TXDATA  REG(0x00800000)   // write: byte to send
#define UART_RXDATA  REG(0x00800004)   // read: received byte
#define UART_STATUS  REG(0x00800008)   // bit0 sending, bit1 byte received, bit2 TX FIFO full
#define UART_CTRL    REG(0x0080000C)   // bit0: interrupt when a byte arrives
#define UART_IRQ     0
// GPIO (LED)
#define GPIO_OUT     REG(0x00803000)
// TIMER0 (interrupt 4)
#define TIMER_LOAD   REG(0x00804000)
#define TIMER_CTRL   REG(0x00804008)   // bit0 enabled, bit1 periodic, bit2 interrupt
#define TIMER_STATUS REG(0x0080400C)   // bit0 expired (write 1 clears)
#define TIMER_IRQ    4
// INTC – interrupt controller
#define INTC_ENABLE  REG(0x00805004)   // bit N = interrupt N enabled
#define INTC_CLAIM   REG(0x00805008)   // interrupt number + 1 (0 = no more requests)

static void uart_putc(char c) {
    while (UART_STATUS & 4) { }        // wait while the TX FIFO is full
    UART_TXDATA = (uint8_t)c;
}

static void uart_puts(const char *s) {
    while (*s) uart_putc(*s++);
}

static void uart_putu(uint32_t v) {
    char buf[11];
    int n = 0;
    do { buf[n++] = (char)('0' + v % 10); v /= 10; } while (v);
    while (n) uart_putc(buf[--n]);
}

// variables changed by the interrupt handler must be volatile (main always reads them from memory)
volatile uint32_t ticks;      // number of timer interrupts
volatile uint32_t rx_count;   // number of received characters

// Interrupt handler: the "interrupt" attribute tells the compiler to save ALL registers it uses
// and to return with the mret instruction (instead of ret). The address of this function is in mtvec (startup.S).
void __attribute__((interrupt("machine"))) trap_handler(void) {
    uint32_t claim;
    // INTC: ask who is requesting an interrupt, as long as there are requests (there may be several at once)
    while ((claim = INTC_CLAIM) != 0) {
        if (claim == TIMER_IRQ + 1) {
            TIMER_STATUS = 1;          // acknowledge the timer interrupt (otherwise it would stay active)
            ticks++;                   // <- a good place for a breakpoint
            GPIO_OUT = ticks;          // LEDs
        } else if (claim == UART_IRQ + 1) {
            while (UART_STATUS & 2) {  // read all received bytes
                char c = (char)UART_RXDATA;
                if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
                uart_putc(c);          // echo in upper case
                rx_count++;
            }
        }
    }
}

int main(void) {
    uart_puts("Interrupts: timer + UART (type text in the console)\n");

    TIMER_LOAD = 2000;                                   // the timer expires every 2000 cycles
    INTC_ENABLE = (1u << TIMER_IRQ) | (1u << UART_IRQ);  // enable those two interrupts in the INTC
    UART_CTRL = 1;                                       // UART: interrupt when a byte arrives
    TIMER_CTRL = 7;                                      // timer: enabled, periodic, with interrupt
    asm volatile("csrsi mstatus, 8");                    // enable interrupts in the processor (mstatus.MIE = 1)

    uint32_t work = 0;
    while (ticks < 20) work++;         // main program – interrupts interrupt it between instructions

    asm volatile("csrci mstatus, 8");  // disable interrupts
    TIMER_CTRL = 0;

    uart_puts("\nDone: ");
    uart_putu(ticks);
    uart_puts(" timer interrupts, ");
    uart_putu(rx_count);
    uart_puts(" characters received, main loop ");
    uart_putu(work);
    uart_puts(" times\n");
    while (UART_STATUS & 1) { }        // wait for the UART to send the last character
    return (int)ticks;                 // 20
}
