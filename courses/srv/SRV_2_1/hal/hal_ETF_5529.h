/**
 * @file    hal_ETF_5529.h
 * @brief   ETF5529 board HAL for the RVisio SoC
 *
 * The examples of the course were written for the ETF5529 board (MSP430F5529). This HAL keeps the
 * same names (LED3, halTOGGLE_LED, vHAL7SEGWriteDigit, HAL_7SEG_DISPLAY_1_ON, ...), but drives the
 * peripherals of the RVisio SoC. The devices of the board are in the simulator panel
 * "Sensors and actuators":
 *
 *   ETF5529 board                      RVisio SoC
 *   LED LD3, LD4 (P2.4, P2.5)          led3, led4 on gpio0 (pins 3, 4)
 *   buttons S2, S3, S4 (P1.1, P1.4, P1.5)  sw2, sw3, sw4 on gpio0 (pins 2, 3, 4)
 *   2 digit 7 segment display          display0 on gpio1 (segments a..g pins 0..6, digits pins 8, 9)
 *   ADC12, inputs A0, A1               adc0, inputs 0 (potentiometer) and 1 (light sensor)
 *   UART USCI_A1 (P4.4, P4.5)          uart0, connected to the Console panel
 */
#ifndef HAL_ETF5529_H
#define HAL_ETF5529_H

#include "soc.h"
#include "hal_led.h"
#include "hal_button.h"
#include "hal_7seg.h"
#include "hal_uart.h"
#include "hal_adc.h"

#endif /* HAL_ETF5529_H */
