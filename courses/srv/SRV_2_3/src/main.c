/**
 * @file    main.c
 * @author  Haris Turkmanovic(haris@etf.rs)
 * @date    2021
 * @brief   SRV Zadatak 7
 *
 * Ported to the RVisio SoC (RISC-V, simulator) from the MSP430F5529 example SRV_2_3
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

/** Task 1 Priority */
#define mainBUTTON_TASK_PRIO        ( 1 )
/** Task 2 Priority */
#define mainLED_TASK_PRIO           ( 2 )

static void prvSetupHardware( void );

/*This semaphore whill be used to signal "Button press" event*/
xSemaphoreHandle    xEvent_ButtonPressed;

/**
 * @brief "Button Task" Function
 *
 * This task detect button press event and signal it to the other task
 * Thought binary semaphore
 */
static void prvButtonTaskFunction( void *pvParameters )
{
    volatile uint16_t i;
    /*Initial button states are 1 because of pull-up configuration*/
    uint8_t     previousButtonState = 1;
    uint8_t     currentButtonState  = 1;
    for ( ;; )
    {
        /*Read button state*/
        currentButtonState = halREAD_BUTTON( HAL_BUTTON_S3 );
        /*Detected button signal edge*/
        if( previousButtonState != currentButtonState){
            for(i = 0; i < 1000; i++);
            /*read button state again to verify rising edge of button signal*/
            currentButtonState = halREAD_BUTTON( HAL_BUTTON_S3 );
            previousButtonState = currentButtonState;
            if(currentButtonState == 0){
                /* If button is still pressed toggle diode LD3 */
                xSemaphoreGive(xEvent_ButtonPressed);
            }
        }
    }
}
/**
 * @brief Task 1 function
 *
 * This task check if button is pressed and change state of diode
 * LD3. At the end, task is blocked for 200 Ticks
 */
static void prvLEDTaskFunction( void *pvParameters )
{
    volatile uint16_t i;
    for ( ;; )
    {
        /*Wait on "Button Pressed" event*/
        xSemaphoreTake(xEvent_ButtonPressed, portMAX_DELAY);
        halTOGGLE_LED( LED3 );
    }
}
void vTaskDelay( TickType_t xTicksToDelay );
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
    xEvent_ButtonPressed    =   xSemaphoreCreateBinary();
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
    /* initialize LEDs */
    vHALInitLED();
    /* Interrupts are enabled when the scheduler starts the first task */
}
