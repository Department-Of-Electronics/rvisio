/**
 * @file    main.c
 * @author  Haris Turkmanovic(haris@etf.rs)
 * @date    2021
 * @brief   SRV Zadatak 11
 *
 * Ported to the RVisio SoC (RISC-V, simulator) from the MSP430F5529 example SRV_2_6
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

typedef enum{
    BUTTON_SW3,
    BUTTON_SW4,
    BUTTON_UNDEF
}button_t;

/* Task priorities */
/** "Button task" priority */
#define mainBUTTON_TASK_PRIO        ( 2 )
/** "LE Diode task" Priority */
#define mainLED_TASK_PRIO           ( 1 )

static void prvSetupHardware( void );
static void vPORT1ISR( BaseType_t *pxHigherPriorityTaskWoken );

/*Used to signal "Button press" event*/
xSemaphoreHandle    xEvent_Button;
/*Used to signal "Change diode state" event*/
xSemaphoreHandle    xEvent_PrintUserString;
/*This semaphore will be used to protect shared resource*/
xSemaphoreHandle    xGuard_PressedButton;

/*This button is used to remember which button is pressed*/
button_t            prvPressedButton;

/**
 * @brief "Button Task" Function
 *
 * This task waits for xEvent_Button semaphore to be given from
 * port ISR. After that, simple debauncing is performed in order
 * to verify which button is pressed and also to verify that button
 * is still pressed
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
        /*Check is SW3 still pressed*/
        currentButtonState = halREAD_BUTTON( HAL_BUTTON_S3 );
        if(currentButtonState == 0){
            /* If button is still pressed write that info to global variable*/
            /* Global variable is shared resource. To access it, first try to
             * take mutex */
            xSemaphoreTake(xGuard_PressedButton, portMAX_DELAY);
            /* Write which button press event is detect */
            prvPressedButton    = BUTTON_SW3;
            /* Give mutex */
            xSemaphoreGive(xGuard_PressedButton);
            /* Signal to "Diode task" to change state */
            xSemaphoreGive(xEvent_PrintUserString);
            continue;
        }
        /*Check is SW4 still pressed*/
        currentButtonState = halREAD_BUTTON( HAL_BUTTON_S4 );
        if(currentButtonState == 0){
            /* If button is still pressed write that info to global variable*/
            /* Global variable is shared resource. To access it, first try to
             * take mutex */
            xSemaphoreTake(xGuard_PressedButton, portMAX_DELAY);
            /* Write which button press event is detect */
            prvPressedButton    = BUTTON_SW4;
            /* Give mutex */
            xSemaphoreGive(xGuard_PressedButton);
            /* Signal to "Diode task" to change state */
            xSemaphoreGive(xEvent_PrintUserString);
            continue;
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
    button_t    pressedButton; /*This variable will be used to temporally store global variable value */
    for ( ;; )
    {
        /*Wait on event*/
        xSemaphoreTake(xEvent_PrintUserString, portMAX_DELAY);
        /* Global variable is shared resource. To access it, first try to
         * take mutex */
        xSemaphoreTake(xGuard_PressedButton, portMAX_DELAY);
        /* Read which button press event is detect */
        pressedButton    = prvPressedButton;
        /* Give mutex */
        xSemaphoreGive(xGuard_PressedButton);
        switch(pressedButton){
        case BUTTON_SW3:
            halTOGGLE_LED(LED3);
            break;
        case BUTTON_SW4:
            halTOGGLE_LED(LED4);
            break;
        case BUTTON_UNDEF:
            //TODO: Implement error detection
            break;
        }
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
    xGuard_PressedButton    =   xSemaphoreCreateMutex();

    prvPressedButton        =   BUTTON_UNDEF;
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
    /* Inputs; interrupt when pressed for: S3, S4 (P1IE/P1IES on the board) */
    vHALInitButtons( HAL_BUTTON_S3 | HAL_BUTTON_S4, HAL_BUTTON_S3 | HAL_BUTTON_S4 );
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
    /* Give semaphore if button SW3 or button SW4 is pressed*/
    /* Note: This check is not truly necessary but it is good to
     * have it*/
    if(((ulIFG & HAL_BUTTON_S3) == HAL_BUTTON_S3) || ((ulIFG & HAL_BUTTON_S4) == HAL_BUTTON_S4)){
        xSemaphoreGiveFromISR(xEvent_Button, pxHigherPriorityTaskWoken);
    }
    /*Clear IFG register on exit: only one flag, if both buttons are pressed the interrupt comes again*/
    halBUTTON_CLEAR_IFG(((ulIFG & HAL_BUTTON_S3) == HAL_BUTTON_S3) ? HAL_BUTTON_S3 : HAL_BUTTON_S4);
}
