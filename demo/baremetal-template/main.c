// main.c, example: output over the UART and a counter on the LEDs.
// Set a breakpoint (click left of the line number), then press F5. The browser shows the whole SoC at the moment it stops.
#include "soc.h"

static void uart_putc(char c) {
    while (UART0_STATUS & 4) { }   // wait while the TX FIFO is full
    UART0_TXDATA = (uint8_t)c;
}

static void uart_puts(const char *s) {
    while (*s) uart_putc(*s++);
}

static volatile uint32_t counter;

int main(void) {
    uart_puts("Hello from VS Code!\n");
    for (int i = 0; i < 16; i++) {
        counter = i;
        GPIO0_OUT = counter;          // LED
    }
    return 0;
}
