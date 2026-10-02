/**
 * @file    hal_uart.h
 * @brief   UART API (ETF5529 board HAL for the RVisio SoC)
 *
 * USCI_A1 of the board (9600 baud to the PC) is uart0 of the RVisio SoC, connected to the Console
 * panel: what the program sends is shown there, and what you type there arrives on RX.
 */
#ifndef HAL_UART_H
#define HAL_UART_H

#include <stdint.h>
#include "soc.h"

/* TX buffer can take a new character (UCA1IFG & UCTXIFG on the board): the TX FIFO is not full */
#define halUART_TX_READY()          ( ( UART_STATUS & 4u ) == 0 )
/* TX buffer (UCA1TXBUF on the board) */
#define halUART_TXBUF               UART_TXDATA
/* RX buffer (UCA1RXBUF on the board): reading takes the received character */
#define halUART_RXBUF               UART_RXDATA

/**
 * @brief Initialize UART
 *
 * @param xRxInterrupt  pdTRUE: interrupt when a character arrives (UCA1IE |= UCRXIE on the board)
 */
static inline void vHALInitUART( uint32_t xRxInterrupt )
{
    UART_CTRL = xRxInterrupt ? 1u : 0u;
}

#endif /* HAL_UART_H */
