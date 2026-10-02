/**
 * @file    main.c
 * @author  Haris Turkmanovic(haris@etf.rs), Strahinja Jankovic (jankovics@etf.bg.ac.rs)
 * @date    2021
 * @brief   SRV Zadatak 13
 *
 * Ported to the RVisio SoC (RISC-V, simulator) from the MSP430F5529 example SRV_2_12
 * (ETF5529 board). Hardware access goes through hal/ (same names as ETF5529_HAL).
 *
 */

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "timers.h"

/* Hardware includes. */
#include "port_soc.h"

/* User's includes */
#include "hal_ETF_5529.h"

/* Task priorities */
/** "Button task" priority */
#define mainBUTTON_TASK_PRIO                ( 1 )

/* Diode Timer Period */
#define mainDIODE_CHANGE_STATE_PERIOD_MS    500

/*This Semaphore will be used to signal potential "Button press" event*/
xSemaphoreHandle    xEvent_Button;

/* Software Timer handler*/
TimerHandle_t       xDiodeTimer;

/* Currently active diode */
uint8_t             xActiveDiode;

static void prvSetupHardware( void );
static void vPORT1ISR( BaseType_t *pxHigherPriorityTaskWoken );

/**
 * @brief "Button Task" Function
 *
 * This task waits for xEvent_Button semaphore to be given from
 * port ISR. After that, simple debauncing is performed in order
 * to verify that button is still pressed. If button is still pressed
 * start timer
 */
static void prvButtonTaskFunction( void *pvParameters )
{
    volatile uint16_t i;
    /*Initial button states are 1 because of pull-up configuration*/
    uint8_t     currentButtonState  = 1;
    for ( ;; )
    {
        /* Before we blocked this task, start time */
        xTimerStart(xDiodeTimer,portMAX_DELAY);
        /* waits for semaphore to be released from ISR*/
        xSemaphoreTake(xEvent_Button,portMAX_DELAY);
        /*wait for a little to check that button is still pressed*/
        for(i = 0; i < 1000; i++);
        /*take button state*/
        currentButtonState = halREAD_BUTTON( HAL_BUTTON_S3 );
        if(currentButtonState == 0){
            /* If button S3 is still pressed first stop time */
            /* it is important to stop timer before change diode state
             * because prvDiodeTimerCallback function also access global
             * variable named xActiveDiode. In order to prevent mutual exclusion
             * we stop timer */
            xTimerStop(xDiodeTimer, portMAX_DELAY);
            /* Change active diode */
            xActiveDiode = xActiveDiode == LED3 ? LED4 : LED3;
        }
    }
}
/**
 * @brief "Diode timer" Callback function
 *
 * This function change diode state
 */
void    prvDiodeTimerCallback(TimerHandle_t xTimer){
    halTOGGLE_LED(xActiveDiode);
}
/**
 * @brief main function
 */
int main( void )
{
    /* Configure peripherals */
    prvSetupHardware();

    /* Create tasks */
    xTaskCreate( prvButtonTaskFunction,
                 "Button Task",
                 configMINIMAL_STACK_SIZE,
                 NULL,
                 mainBUTTON_TASK_PRIO,
                 NULL
               );
    /* Create timer */
    xDiodeTimer         = xTimerCreate("Diode timer",
                                       pdMS_TO_TICKS(mainDIODE_CHANGE_STATE_PERIOD_MS),
                                       pdTRUE,
                                       NULL,
                                       prvDiodeTimerCallback);
    /*Create semaphores*/
    xEvent_Button           =   xSemaphoreCreateBinary();
    /* Set user variables */
    xActiveDiode    = LED3;
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
    /* Inputs; interrupt when pressed for: S3 (P1IE/P1IES on the board) */
    vHALInitButtons( HAL_BUTTON_S3 | HAL_BUTTON_S4, HAL_BUTTON_S3 );
    /* Button interrupt handler (PORT1_VECTOR on the board) */
    vPortSetInterruptHandler( GPIO_IRQ, vPORT1ISR );
    /* initialize LEDs */
    vHALInitLED();
    /* Interrupts are enabled when the scheduler starts the first task */
}

/**
 * @brief Button interrupt (on the ETF5529 board: PORT1_VECTOR)
 *
 * Registered with vPortSetInterruptHandler() in prvSetupHardware. The port calls it in interrupt
 * context and switches to the woken task when the interrupt ends if *pxHigherPriorityTaskWoken is
 * pdTRUE (on the MSP430 this was done by portYIELD_FROM_ISR at the end of the routine).
 */
static void vPORT1ISR( BaseType_t *pxHigherPriorityTaskWoken )
{
    /* Buttons pressed since their flag was cleared (P1IFG on the board) */
    uint32_t ulIFG = halBUTTON_IFG();
    /* Give semaphore if button SW3 is pressed*/
    /* Note: This check is not truly necessary but it is good to
     * have it*/
    if((ulIFG & HAL_BUTTON_S3) == HAL_BUTTON_S3){
        xSemaphoreGiveFromISR(xEvent_Button, pxHigherPriorityTaskWoken);
    }
    /*Clear IFG register on exit*/
    halBUTTON_CLEAR_IFG(HAL_BUTTON_S3);
}
