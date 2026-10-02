/**
 * @file    hal_7seg.c
 * @brief   7SEG DISPLAY API (ETF5529 board HAL for the RVisio SoC)
 */
#include "hal_7seg.h"

/* Segments that are on for digits 0..9 (bit 0 = a ... bit 6 = g) */
static const uint8_t prvDigitSegments[ 10 ] = {
    0x3F,   /* 0: a b c d e f   */
    0x06,   /* 1:   b c         */
    0x5B,   /* 2: a b   d e   g */
    0x4F,   /* 3: a b c d     g */
    0x66,   /* 4:   b c     f g */
    0x6D,   /* 5: a   c d   f g */
    0x7D,   /* 6: a   c d e f g */
    0x07,   /* 7: a b c         */
    0x7F,   /* 8: a b c d e f g */
    0x6F,   /* 9: a b c d   f g */
};

void vHAL7SEGInit( void )
{
    /* Display and segment pins are outputs */
    HAL_7SEG_DIR |= HAL_7SEG_DISPLAY_1_MASK | HAL_7SEG_DISPLAY_2_MASK | HAL_7SEG_SEGMENT_ALL;
    /* On the ETF5529 board the outputs are 0 after reset and 0 turns a display and a segment on
     * (active low), so both displays show "8" until the program changes them. Same here. */
    HAL_7SEG_OUT |= HAL_7SEG_DISPLAY_1_MASK | HAL_7SEG_DISPLAY_2_MASK | HAL_7SEG_SEGMENT_ALL;
}

uint8_t vHAL7SEGWriteDigit( uint8_t digit )
{
    if( digit > 9 ){
        return 1;
    }
    /* Change only the segment lines, the display enable pins stay as they are */
    HAL_7SEG_OUT = ( HAL_7SEG_OUT & ~HAL_7SEG_SEGMENT_ALL ) | prvDigitSegments[ digit ];
    return 0;
}
