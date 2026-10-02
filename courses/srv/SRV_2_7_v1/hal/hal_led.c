/**
 * @file    hal_led.c
 * @brief   LED API (ETF5529 board HAL for the RVisio SoC)
 */
#include "hal_led.h"

void vHALInitLED( void )
{
    /* Switch pins to output */
    LED_PORT_DIR |= LED_ALL;
    /* Switch LEDs off initially */
    LED_PORT_OUT &= ~LED_ALL;
}
