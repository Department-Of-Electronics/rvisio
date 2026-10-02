/**
 * @file    main.c
 * @author  Haris Turkmanovic(haris@etf.rs)
 * @date    2021
 * @brief   SRV Zadatak 17
 *
 * Ported to the RVisio SoC (RISC-V, simulator) from the MSP430F5529 example SRV_2_10
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

typedef enum{
    DIODE_COMMAND_ON,
    DIODE_COMMAND_OFF,
    DIODE_COMMAND_UNDEF
}diode_command_t;

/* Task priorities */
/** "Button processing" priority */
#define mainBUTTON_PROCESSING_TASK_PRIO     ( 3 )
/** "Char processing" priority */
#define mainCHAR_PROCESSING_TASK_PRIO       ( 1 )
/** "LE Diode task" Priority */
#define mainDIODE_CONTROL_TASK_PRIO         ( 2 )

/* Char queue parameters value*/
#define mainCHAR_QUEUE_LENGTH               5
/* Diode command queue parameters value*/
#define mainDIODE_COMMAND_QUEUE_LENGTH      5

static void prvSetupHardware( void );
static void vPORT1ISR( BaseType_t *pxHigherPriorityTaskWoken );
static void vUARTISR( BaseType_t *pxHigherPriorityTaskWoken );

/* This semaphore will be used to signal "Button press" event*/
xSemaphoreHandle    xEvent_Button;
/* This queue will be used to buffer char received over UART interface*/
xQueueHandle        xCharQueue;
/* This queue will be used to buffer diode control messages*/
xQueueHandle        xCommandQueue;
/**
 * @brief "Char Processing" Function
 *
 * This task waits for character over UART received from xCharQueue. After character
 * is received it is processed and based on character value appropriate Diode Control
 * message is written to global variable
 */
static void prvCharProcessingTaskFunction( void *pvParameters )
{
    char        recChar =   0;
    diode_command_t commandToSend = DIODE_COMMAND_UNDEF;
    for ( ;; )
    {
        /*Read char from the queue*/
        xQueueReceive(xCharQueue, &recChar, portMAX_DELAY);
        /*Determine which character is received and based on character value
         * determine which command will be sent to "Diode Control" task*/
        switch(recChar){
            case 'e':
                commandToSend   = DIODE_COMMAND_ON;
                break;
            case 'd':
                commandToSend   = DIODE_COMMAND_OFF;
                break;
            default:
                commandToSend   = DIODE_COMMAND_UNDEF;
                break;
        }
        if(commandToSend != DIODE_COMMAND_UNDEF)
            xQueueSendToBack(xCommandQueue,&commandToSend,portMAX_DELAY);
    }
}
/**
 * @brief "Diode Control" task function
 *
 * This task set diode state based on prvDIODE_COMMAND variable
 */
static void prvDiodeControlTaskFunction( void *pvParameters )
{
    diode_command_t commandToProcess = DIODE_COMMAND_UNDEF;
    for ( ;; )
    {
        /* Wait on command*/
        xQueueReceive(xCommandQueue, &commandToProcess, portMAX_DELAY);
        /* Process command*/
        switch(commandToProcess){
            case DIODE_COMMAND_OFF:
                halCLR_LED(LED3);
                break;
            case DIODE_COMMAND_ON:
                halSET_LED(LED3);
                break;
            case DIODE_COMMAND_UNDEF:
                break;
        }
    }
}
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
    uint8_t         currentButtonState  = 1;
    diode_command_t commandToSend       = DIODE_COMMAND_UNDEF;
    for ( ;; )
    {
        xSemaphoreTake(xEvent_Button,portMAX_DELAY);
        /*wait for a little to check that button is still pressed*/
        for(i = 0; i < 1000; i++);
        /*take button state*/
        /* check if button SW3 is pressed*/
        currentButtonState = halREAD_BUTTON( HAL_BUTTON_S3 );
        if(currentButtonState == 0){
            /* If SW3 is pressed send command to disable diode */
            commandToSend   =   DIODE_COMMAND_OFF;
            xQueueSendToBack(xCommandQueue, &commandToSend, portMAX_DELAY);
            continue;
        }
        /* check if button SW4 is pressed*/
        currentButtonState = halREAD_BUTTON( HAL_BUTTON_S4 );
        if(currentButtonState == 0){
            /* If SW4 is pressed send command to enable diode */
            commandToSend   =   DIODE_COMMAND_ON;
            xQueueSendToBack(xCommandQueue, &commandToSend, portMAX_DELAY);
            continue;
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
    xTaskCreate( prvCharProcessingTaskFunction,
                 "Char Processing Task",
                 configMINIMAL_STACK_SIZE,
                 NULL,
                 mainCHAR_PROCESSING_TASK_PRIO,
                 NULL
               );
    xTaskCreate( prvDiodeControlTaskFunction,
                 "Diode Control Task",
                 configMINIMAL_STACK_SIZE,
                 NULL,
                 mainDIODE_CONTROL_TASK_PRIO,
                 NULL
               );
    xTaskCreate( prvButtonTaskFunction,
                 "Button Processing Task",
                 configMINIMAL_STACK_SIZE,
                 NULL,
                 mainBUTTON_PROCESSING_TASK_PRIO,
                 NULL
               );
    /* Create FreeRTOS objects  */
    /* Create semaphores        */
    xEvent_Button           =   xSemaphoreCreateBinary();
    /* Create Queue*/
    xCharQueue              =   xQueueCreate(mainCHAR_QUEUE_LENGTH,sizeof(char));
    xCommandQueue           =   xQueueCreate(mainDIODE_COMMAND_QUEUE_LENGTH,sizeof(diode_command_t));
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
    /* Initialize UART (uart0, Console panel): interrupt when a character arrives (UCRXIE) */
    vHALInitUART( pdTRUE );
    vPortSetInterruptHandler( UART_IRQ, vUARTISR );
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
    /*Clear IFG register on exit*/
    halBUTTON_CLEAR_IFG(HAL_BUTTON_S3 | HAL_BUTTON_S4);
}

/**
 * @brief UART RX interrupt (on the ETF5529 board: USCI_A1_VECTOR)
 *
 * Registered with vPortSetInterruptHandler() in prvSetupHardware. The port calls it in interrupt
 * context and switches to the woken task when the interrupt ends if *pxHigherPriorityTaskWoken is
 * pdTRUE (on the MSP430 this was done by portYIELD_FROM_ISR at the end of the routine).
 */
static void vUARTISR( BaseType_t *pxHigherPriorityTaskWoken )
{
    /* RX: a character arrived (UCA1IV = 2, RXIFG on the board); take every received character */
    while( UART_STATUS & 2u ){
        char cReceived = ( char ) halUART_RXBUF;
        xQueueSendToBackFromISR(xCharQueue, &cReceived, pxHigherPriorityTaskWoken);
    }
}
