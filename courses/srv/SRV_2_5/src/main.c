/**
 * @file    main.c
 * @author  Haris Turkmanovic(haris@etf.rs)
 * @date    2021
 * @brief   SRV Zadatak 9
 *
 * Ported to the RVisio SoC (RISC-V, simulator) from the MSP430F5529 example SRV_2_5
 * (ETF5529 board). Hardware access goes through hal/ (same names as ETF5529_HAL).
 *
 */

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* Hardware includes. */
#include "port_soc.h"

/* User's includes */
#include "hal_ETF_5529.h"

/* Task priorities */
/** "Button task" priority */
#define mainBUTTON_TASK_PRIO        ( 2 )
/** "LE Diode task" Priority */
#define mainLED_TASK_PRIO           ( 1 )

static void prvSetupHardware( void );
static void vPORT1ISR( BaseType_t *pxHigherPriorityTaskWoken );

/*This semaphore will be used to signal "Button press" event*/
xSemaphoreHandle    xEvent_Button;
/*This semaphore will be used to signal "Change diode state" event*/
xSemaphoreHandle    xEvent_PrintUserString;

/**
 * @brief "Button Task" Function
 *
 * This task waits for xEvent_Button semaphore to be given from
 * port ISR. After that, simple debauncing is performed in order
 * to verify that button is still pressed
 */
static void prvButtonTaskFunction( void *pvParameters )
{
    volatile uint16_t i;
    /*Initial button states are 1 because of pull-up configuration*/
    uint8_t     currentButtonState  = 1;
    for ( ;; )
    {
        xSemaphoreTake(xEvent_Button,portMAX_DELAY);
        /*wait for a little to check that button is still pressed*/
        for(i = 0; i < 1000; i++);
        /*take button state*/
        currentButtonState = halREAD_BUTTON( HAL_BUTTON_S3 );
        if(currentButtonState == 0){
            /* If button is still pressed send signal to "Diode" task */
            xSemaphoreGive(xEvent_PrintUserString);
        }
    }
}
/**
 * @brief "LE Diode Task" function
 *
 * This task wait on "Counting" event and change LD3 diode state
 */
static void prvLEDTaskFunction( void *pvParameters )
{
    volatile uint16_t i;
    for ( ;; )
    {
        /*Wait on event*/
        xSemaphoreTake(xEvent_PrintUserString, portMAX_DELAY);
        halTOGGLE_LED( LED3 );
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
    xTaskCreate( prvButtonTaskFunction,
                 "Button Task",
                 configMINIMAL_STACK_SIZE,
                 NULL,
                 mainBUTTON_TASK_PRIO,
                 NULL
               );
    xTaskCreate( prvLEDTaskFunction,
                 "LED Task",
                 configMINIMAL_STACK_SIZE,
                 NULL,
                 mainLED_TASK_PRIO,
                 NULL
               );
    /*Create FreeRTOS objects*/
    /*Create semaphores*/
    xEvent_Button           =   xSemaphoreCreateBinary();
    xEvent_PrintUserString            =   xSemaphoreCreateBinary();
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
