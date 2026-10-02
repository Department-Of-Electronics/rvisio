/**
 * @file    main.c
 * @author  Haris Turkmanovic(haris@etf.rs)
 * @date    2021
 * @brief   SRV Zadatak 18
 *
 * Ported to the RVisio SoC (RISC-V, simulator) from the MSP430F5529 example SRV_2_11
 * (ETF5529 board). Hardware access goes through hal/ (same names as ETF5529_HAL).
 *
 */

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"

/* Hardware includes. */
#include "port_soc.h"

/* User's includes */
#include "hal_ETF_5529.h"

/** "Display task" priority */
#define mainDISPLAY_TASK_PRIO           ( 1 )
/** "ADC task" priority */
#define mainADC_TASK_PRIO               ( 2 )

/* Display queue parameters value*/
/* Queue with length 1 is mailbox*/
#define mainDISPLAY_QUEUE_LENGTH            1

static void prvSetupHardware( void );
static void vADC12ISR( BaseType_t *pxHigherPriorityTaskWoken );

/* This queue will be used to send data to display task*/
xQueueHandle        xDisplayMailbox;
/**
 * @brief "Display Task" Function
 *
 * This task read data from xDisplayQueue but reading is not blocking.
 * xDisplayQueue is used to send data which will be printed on 7Seg
 * display. After data is received it is decomposed on high and low digit.
 */
static void prvDisplayTaskFunction( void *pvParameters )
{
    /* New received 8bit data */
    uint8_t         NewValueToShow      = 0;
    /* High and Low number digit*/
    uint8_t         digitLow, digitHigh;
    for ( ;; )
    {
        /* Check if new number is received*/
        if(xQueueReceive(xDisplayMailbox, &NewValueToShow, 0) == pdTRUE){
            /* If there is new number to show on display, split it on High and Low digit */
            /* Extract high digit*/
            digitHigh = NewValueToShow/10;
            /* Extract low digit*/
            digitLow = NewValueToShow - digitHigh*10;
        }
        HAL_7SEG_DISPLAY_1_ON;
        HAL_7SEG_DISPLAY_2_OFF;
        vHAL7SEGWriteDigit(digitLow);
        vTaskDelay(5);
        HAL_7SEG_DISPLAY_2_ON;
        HAL_7SEG_DISPLAY_1_OFF;
        vHAL7SEGWriteDigit(digitHigh);
        vTaskDelay(5);
    }
}

/**
 * @brief "ADC Task" Function
 *
 * This task periodically, with 200 sys-tick period, trigger ADC
 */
static void prvADCTaskFunction( void *pvParameters )
{

    for ( ;; )
    {
       /*Trigger ADC Conversion*/
       halADC_START();
       vTaskDelay(200);
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
    xTaskCreate( prvDisplayTaskFunction,
                 "Display Task",
                 configMINIMAL_STACK_SIZE,
                 NULL,
                 mainDISPLAY_TASK_PRIO,
                 NULL
               );
    xTaskCreate( prvADCTaskFunction,
                 "ADC Task",
                 configMINIMAL_STACK_SIZE,
                 NULL,
                 mainADC_TASK_PRIO,
                 NULL
               );
    /* Create FreeRTOS objects  */
    xDisplayMailbox       =   xQueueCreate(mainDISPLAY_QUEUE_LENGTH,sizeof(uint8_t));
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
    /* - Init buttons - */
    /* Inputs; interrupt when pressed for: none (P1IE/P1IES on the board) */
    vHALInitButtons( HAL_BUTTON_S3 | HAL_BUTTON_S4, 0 );
    /*Initialize ADC: on, interrupt at the end of a conversion, input 0 */
    vHALInitADC( 0 );
    vPortSetInterruptHandler( ADC_IRQ, vADC12ISR );
    /* initialize LEDs */
    vHALInitLED();
    /* initialize display*/
    vHAL7SEGInit();
    /* Interrupts are enabled when the scheduler starts the first task */
}

/**
 * @brief ADC end of conversion interrupt (on the ETF5529 board: ADC12_VECTOR, ADC12IFG0)
 *
 * Registered with vPortSetInterruptHandler() in prvSetupHardware. The port calls it in interrupt
 * context and switches to the woken task when the interrupt ends if *pxHigherPriorityTaskWoken is
 * pdTRUE (on the MSP430 this was done by portYIELD_FROM_ISR at the end of the routine).
 */
static void vADC12ISR( BaseType_t *pxHigherPriorityTaskWoken )
{
    uint16_t    temp;
    /* Scaling ADC value to fit on two digits representation (12 bit result -> 0..63);
     * reading the result clears the interrupt flag */
    temp    = halADC_RESULT()>>6;
    xQueueSendToBackFromISR(xDisplayMailbox,&temp,pxHigherPriorityTaskWoken);
}
