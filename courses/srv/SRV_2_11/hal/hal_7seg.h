/**
 * @file    hal_7seg.h
 * @brief   7SEG DISPLAY API (ETF5529 board HAL for the RVisio SoC)
 *
 * The 2 digit display is display0 in the "Sensors and actuators" panel. Both digits share the
 * segment lines a..g (gpio1 pins 0..6); pin 8 turns on display 1 (right digit) and pin 9 turns on
 * display 2 (left digit). Only one digit can show its own value at a time, so the program switches
 * between them (multiplexing); if it does that often enough, the eye sees both digits at once.
 */
#ifndef HAL_7SEG_H_
#define HAL_7SEG_H_

#include <stdint.h>
#include "soc.h"

/* Displays position inside of register */
#define HAL_7SEG_DISPLAY_1_MASK         ( 1u << 8 )
#define HAL_7SEG_DISPLAY_2_MASK         ( 1u << 9 )

/* Segments position inside of register */
#define HAL_7SEG_SEGMENT_A_MASK         ( 1u << 0 )
#define HAL_7SEG_SEGMENT_B_MASK         ( 1u << 1 )
#define HAL_7SEG_SEGMENT_C_MASK         ( 1u << 2 )
#define HAL_7SEG_SEGMENT_D_MASK         ( 1u << 3 )
#define HAL_7SEG_SEGMENT_E_MASK         ( 1u << 4 )
#define HAL_7SEG_SEGMENT_F_MASK         ( 1u << 5 )
#define HAL_7SEG_SEGMENT_G_MASK         ( 1u << 6 )
#define HAL_7SEG_SEGMENT_ALL            ( 0x7Fu )

/* Display and segments are on GPIO1 */
#define HAL_7SEG_DIR                    GPIO1_DIR
#define HAL_7SEG_OUT                    GPIO1_OUT

#define HAL_7SEG_DISPLAY_1_ON           ( HAL_7SEG_OUT |=  HAL_7SEG_DISPLAY_1_MASK )
#define HAL_7SEG_DISPLAY_2_ON           ( HAL_7SEG_OUT |=  HAL_7SEG_DISPLAY_2_MASK )
#define HAL_7SEG_DISPLAY_1_OFF          ( HAL_7SEG_OUT &= ~HAL_7SEG_DISPLAY_1_MASK )
#define HAL_7SEG_DISPLAY_2_OFF          ( HAL_7SEG_OUT &= ~HAL_7SEG_DISPLAY_2_MASK )

typedef enum{
    HAL_DISPLAY_1 = 0,
    HAL_DISPLAY_2 = 1
}hal_7seg_display_t;

/* Init 7seg displays and segments: as on the board after reset, both displays and all segments are on ("88") */
void        vHAL7SEGInit( void );
/* Write digit to previously enabled display (0..9), returns 1 for other values */
uint8_t     vHAL7SEGWriteDigit( uint8_t digit );

#endif /* HAL_7SEG_H_ */
