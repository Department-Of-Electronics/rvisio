/**
 * @file    hal_adc.h
 * @brief   ADC API (ETF5529 board HAL for the RVisio SoC)
 *
 * ADC12 of the board is adc0 of the RVisio SoC (12 bit result, 0..4095). Input 0 is the
 * potentiometer and input 1 the light sensor in the "Sensors and actuators" panel: move the slider
 * to change the voltage. A conversion is started by the program and its end is signalled with an
 * interrupt (ADC_IRQ), like ADC12IFG0 on the board.
 */
#ifndef HAL_ADC_H
#define HAL_ADC_H

#include <stdint.h>
#include "soc.h"

/**
 * @brief Initialize ADC: on, interrupt at the end of a conversion, selected input
 * (ADC12CTL0 = ADC12ON, ADC12IE = 0x01, ADC12MCTL0 = ADC12INCH_x on the board)
 */
static inline void vHALInitADC( uint32_t ulChannel )
{
    ADC_CTRL = ADC_CTRL_ON | ADC_CTRL_IE | ( ( ulChannel & 7u ) << 4 );
}

/* Start a conversion (ADC12CTL0 |= ADC12SC on the board) */
#define halADC_START()                  ( ADC_START = 1u )
/* Select the input for the next conversions (ADC12MCTL0 = ADC12INCH_x on the board) */
#define halADC_SELECT_CHANNEL( ch )     ( ADC_CTRL = ( ADC_CTRL & ~0x70u ) | ( ( ( ch ) & 7u ) << 4 ) )
/* Result of the last conversion, 0..4095 (ADC12MEM0 on the board): reading clears the interrupt flag */
#define halADC_RESULT()                 ( ADC_DATA )

#endif /* HAL_ADC_H */
