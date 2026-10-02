/* soc.h – addresses and registers of the simulator's default SoC ("Memory map" panel). */
#ifndef SOC_H
#define SOC_H

#include <stdint.h>

#define REG32(a) (*(volatile uint32_t *)(a))

/* memories */
#define FLASH_BASE    0x00000000u   /* 64 KB, code and constants */
#define SRAM_BASE     0x00010000u   /* 64 KB, data, heap and stacks */

/* UART0 – interrupt 0 */
#define UART0_BASE    0x00800000u
#define UART_TXDATA   REG32(UART0_BASE + 0x00)  /* write: byte to send */
#define UART_RXDATA   REG32(UART0_BASE + 0x04)  /* read: received byte */
#define UART_STATUS   REG32(UART0_BASE + 0x08)  /* bit0 sending, bit1 byte received, bit2 TX FIFO full */
#define UART_CTRL     REG32(UART0_BASE + 0x0C)  /* bit0: interrupt when a byte arrives */
#define UART_IRQ      0

/* UART1 – second UART, interrupt 6 (same registers as UART0) */
#define UART1_BASE    0x00807000u
#define UART1_IRQ     6

/* GPIO0 – LEDs (OUT) and buttons (IN), interrupt 3 */
#define GPIO_BASE       0x00803000u
#define GPIO_OUT        REG32(GPIO_BASE + 0x00)  /* outputs: LEDs */
#define GPIO_IN         REG32(GPIO_BASE + 0x04)  /* inputs: buttons (1 = pressed) */
#define GPIO_DIR        REG32(GPIO_BASE + 0x08)
#define GPIO_IRQ_EN     REG32(GPIO_BASE + 0x0C)  /* bit N: interrupt on change of input N */
#define GPIO_IRQ_STATUS REG32(GPIO_BASE + 0x10)  /* bit N: input N changed (write 1 clears) */
#define GPIO_IRQ        3

/* TIMER0 – interrupt 4 (FreeRTOS tick) */
#define TIMER0_BASE   0x00804000u
#define TIMER_LOAD    REG32(TIMER0_BASE + 0x00)
#define TIMER_VALUE   REG32(TIMER0_BASE + 0x04)
#define TIMER_CTRL    REG32(TIMER0_BASE + 0x08) /* bit0 enabled, bit1 periodic, bit2 interrupt */
#define TIMER_STATUS  REG32(TIMER0_BASE + 0x0C) /* bit0 expired (write 1 clears) */
#define TIMER_IRQ     4

/* INTC – interrupt controller */
#define INTC_BASE     0x00805000u
#define INTC_PENDING  REG32(INTC_BASE + 0x00)
#define INTC_ENABLE   REG32(INTC_BASE + 0x04)   /* bit N = interrupt N enabled */
#define INTC_CLAIM    REG32(INTC_BASE + 0x08)   /* number+1 of the lowest active interrupt, 0 = none */
#define INTC_SOFT     REG32(INTC_BASE + 0x0C)   /* software interrupt 31: 1 = request, 0 = clear */
#define SOFT_IRQ      31

#endif
