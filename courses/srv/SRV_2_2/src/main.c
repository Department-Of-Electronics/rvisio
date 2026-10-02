/**
 * @file    main.c
 * @author  Haris Turkmanovic(haris@etf.rs)
 * @date    2022
 * @brief   SRV Zadatak 6
 *
 * Ported to the RVisio SoC (RISC-V, simulator) from the MSP430F5529 example SRV_2_2
 * (ETF5529 board). Hardware access goes through hal/ (same names as ETF5529_HAL).
 *
 *
 */

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"

/* Hardware includes. */
#include "port_soc.h"

/* User's includes */
#include "hal_ETF_5529.h"

/** Task 1 Priority */
#define mainTAKS_1_PRIO        ( 1 )

static void     prvSetupHardware( void );
static void     prvTask1Function( void *pvParameters );

/* This variable will be printed on 2digit 7 seg display*/
static uint8_t              data;

/**
 * @brief Task 1 function
 *
 * This task implements 2 digit 7seg multiplexing functionality
 *
 */
static void prvTask1Function( void *pvParameters )
{
    hal_7seg_display_t xCurrentActiveDisplay = HAL_DISPLAY_1;
    uint8_t xSecondDigit     = data / 10;
    uint8_t xFirstDigit    = data - xSecondDigit*10;
    for ( ;; )
    {
        switch(xCurrentActiveDisplay){
            case HAL_DISPLAY_1:
                HAL_7SEG_DISPLAY_1_ON;
                HAL_7SEG_DISPLAY_2_OFF;
                /* Extract first digit from number*/
                xFirstDigit    = data - xSecondDigit*10;
                /* Show digit on previously enabled display*/
                vHAL7SEGWriteDigit(xFirstDigit);
                /* Change active display*/
                xCurrentActiveDisplay = HAL_DISPLAY_2;
                break;
            case HAL_DISPLAY_2:
                HAL_7SEG_DISPLAY_1_OFF;
                HAL_7SEG_DISPLAY_2_ON;
                /* Extract first digit from number*/
                xSecondDigit = data / 10;
                /* Show digit on previously enabled display*/
                vHAL7SEGWriteDigit(xSecondDigit);
                /* Change active display*/
                xCurrentActiveDisplay = HAL_DISPLAY_1;
                break;
        }
        vTaskDelay( pdMS_TO_TICKS(5) );
    }
}
/**
 * @brief main function
 */
int main( void )
{
    /* Configure peripherals */
    prvSetupHardware();

    /* Create tasks */
    if(xTaskCreate( prvTask1Function,
                 "Task 1",
                 configMINIMAL_STACK_SIZE,
                 NULL,
                 mainTAKS_1_PRIO,
                 NULL
               ) != pdPASS) while(1);

    /* Start the scheduler. */
    vTaskStartScheduler();

    /* If all is well then this line will never be reached.  If it is reached
    then it is likely that there was insufficient (FreeRTOS) heap memory space
    to create the idle task.  This may have been trapped by the malloc() failed
    hook function, if one is configured. */	
    for( ;; );
}

/**
 * @brief Configure hardware upon boot
 */
static void prvSetupHardware( void )
{
    taskDISABLE_INTERRUPTS();
    /* Watchdog and clock setup of the MSP430 are not needed on the RVisio SoC */
    /* initialize LEDs */
    vHALInitLED();
    /* initialize display*/
    vHAL7SEGInit();
    /* Interrupts are enabled when the scheduler starts the first task */
}
