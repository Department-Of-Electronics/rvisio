/* soc.h – addresses and registers of the simulator's default SoC ("Memory map" panel, "📄 Datasheet"). */
#ifndef SOC_H
#define SOC_H

#include <stdint.h>

#define REG32(a) (*(volatile uint32_t *)(a))

/* memories */
#define FLASH_BASE    0x00000000u   /* 64 KB, code and constants */
#define SRAM_BASE     0x00010000u   /* 64 KB, data, heap and stacks */

/* UART0 – interrupt 0, connected to the Console panel */
#define UART0_BASE    0x00800000u
#define UART_TXDATA   REG32(UART0_BASE + 0x00)  /* write: byte to send */
#define UART_RXDATA   REG32(UART0_BASE + 0x04)  /* read: received byte */
#define UART_STATUS   REG32(UART0_BASE + 0x08)  /* bit0 sending, bit1 byte received, bit2 TX FIFO full */
#define UART_CTRL     REG32(UART0_BASE + 0x0C)  /* bit0: interrupt when a byte arrives */
#define UART_IRQ      0

/* GPIO0 – 8 pins: LEDs led0..led7 (OUT) and buttons sw0..sw7 (IN, 1 = pressed), interrupt 3 */
#define GPIO_BASE       0x00803000u
#define GPIO_OUT        REG32(GPIO_BASE + 0x00)  /* outputs */
#define GPIO_IN         REG32(GPIO_BASE + 0x04)  /* inputs */
#define GPIO_DIR        REG32(GPIO_BASE + 0x08)
#define GPIO_IRQ_EN     REG32(GPIO_BASE + 0x0C)  /* bit N: interrupt on change of input N */
#define GPIO_IRQ_STATUS REG32(GPIO_BASE + 0x10)  /* bit N: input N changed (write 1 clears) */
#define GPIO_IRQ        3

/* GPIO1 – 16 pins, no interrupt: 7 segment display0 (segments a..g, dp on pins 0..7, digits on pins 8, 9) */
#define GPIO1_BASE      0x00808000u
#define GPIO1_OUT       REG32(GPIO1_BASE + 0x00)
#define GPIO1_IN        REG32(GPIO1_BASE + 0x04)
#define GPIO1_DIR       REG32(GPIO1_BASE + 0x08)

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

/* ADC0 – 12 bit, 2 inputs (0: potentiometer, 1: light sensor), interrupt 7 */
#define ADC_BASE      0x00809000u
#define ADC_CTRL      REG32(ADC_BASE + 0x00)    /* bit0 ON, bit1 interrupt enable, bits 4..6 input */
#define ADC_START     REG32(ADC_BASE + 0x04)    /* write 1: start a conversion */
#define ADC_STATUS    REG32(ADC_BASE + 0x08)    /* bit0 done (write 1 clears), bit1 busy, bit2 overrun */
#define ADC_DATA      REG32(ADC_BASE + 0x0C)    /* result 0..4095; reading clears "done" */
#define ADC_CTRL_ON   0x01u
#define ADC_CTRL_IE   0x02u
#define ADC_IRQ       7

#endif
