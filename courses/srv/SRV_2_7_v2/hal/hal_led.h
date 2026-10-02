/**
 * @file    hal_led.h
 * @brief   LED API (ETF5529 board HAL for the RVisio SoC)
 *
 * LEDs LD3 and LD4 are led3 and led4 in the "Sensors and actuators" panel, on gpio0 pins 3 and 4.
 * A LED is on while its output is 1.
 */
#ifndef HAL_LED_H
#define HAL_LED_H

#include "soc.h"

/* LEDs are connected to GPIO0 */
#define LED_PORT_DIR        ( GPIO_DIR )
#define LED_PORT_OUT        ( GPIO_OUT )

/* LED pins definitions */
#define LED3_BIT            ( 3 )
#define LED4_BIT            ( 4 )

/* macros to be used in code */
#define LED3                ( 1 << LED3_BIT )
#define LED4                ( 1 << LED4_BIT )
#define LED_ALL             ( LED4 | LED3 )

/**
 * @brief Initialize LED pins: outputs, LEDs off
 */
extern void vHALInitLED( void );

/* LED management macros */
/* Turn on LED */
#define halSET_LED( led )       ( LED_PORT_OUT |= ( led ) )
/* Turn off LED */
#define halCLR_LED( led )       ( LED_PORT_OUT &= ~( led ) )
/* Toggle LED */
#define halTOGGLE_LED( led )    ( LED_PORT_OUT ^= ( led ) )

#endif /* HAL_LED_H */
