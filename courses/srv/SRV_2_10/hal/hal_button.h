/**
 * @file    hal_button.h
 * @brief   Button API (ETF5529 board HAL for the RVisio SoC)
 *
 * Buttons S2, S3 and S4 are sw2, sw3 and sw4 in the "Sensors and actuators" panel, on gpio0 pins
 * 2, 3 and 4 (hold the mouse button = button pressed).
 *
 * On the ETF5529 board the buttons have pull-up resistors: the pin reads 1 when the button is released
 * and 0 while it is pressed, and the port interrupt (P1IFG) is set on the 1 -> 0 edge. On the RVisio
 * SoC the GPIO input is 1 while the button is pressed and the interrupt comes on every change, so the
 * macros below give the same values as on the board: halREAD_BUTTON() is 0 while pressed and
 * halBUTTON_IFG() reports only presses.
 */
#ifndef HAL_BUTTON_H
#define HAL_BUTTON_H

#include <stdint.h>
#include "soc.h"

/* Button pins on GPIO0 */
#define HAL_BUTTON_S2           ( 1u << 2 )
#define HAL_BUTTON_S3           ( 1u << 3 )
#define HAL_BUTTON_S4           ( 1u << 4 )
#define HAL_BUTTON_ALL          ( HAL_BUTTON_S2 | HAL_BUTTON_S3 | HAL_BUTTON_S4 )

/* Button state as on the ETF5529 board (pull-up): 0 = pressed, 1 = released */
#define halREAD_BUTTON( b )         ( ( GPIO_IN & ( b ) ) ? 0u : 1u )

/**
 * @brief Initialize button pins
 *
 * @param ulButtons     buttons to use (HAL_BUTTON_S2 | HAL_BUTTON_S3 | ...)
 * @param ulInterrupts  buttons that request an interrupt when pressed (P1IE on the board), 0 = none
 */
static inline void vHALInitButtons( uint32_t ulButtons, uint32_t ulInterrupts )
{
    /* Set direction to input */
    GPIO_DIR &= ~ulButtons;
    /* Clear old interrupt flags and enable the interrupt for the selected buttons */
    GPIO_IRQ_STATUS = ulButtons;
    GPIO_IRQ_EN = ( GPIO_IRQ_EN & ~ulButtons ) | ( ulInterrupts & ulButtons );
}

/**
 * @brief Interrupt flags of the buttons (P1IFG on the board)
 *
 * Returns the buttons that were pressed since their flag was cleared. Flags of released buttons
 * are cleared here: on the board a release does not request an interrupt.
 */
static inline uint32_t halBUTTON_IFG( void )
{
    const uint32_t ulStatus = GPIO_IRQ_STATUS & HAL_BUTTON_ALL;
    const uint32_t ulPressed = ulStatus & GPIO_IN;
    GPIO_IRQ_STATUS = ulStatus & ~ulPressed;
    return ulPressed;
}

/* Clear interrupt flags of the given buttons (P1IFG &= ~b on the board) */
#define halBUTTON_CLEAR_IFG( b )    ( GPIO_IRQ_STATUS = ( b ) )

#endif /* HAL_BUTTON_H */
