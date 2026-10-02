/**
 * @file    main.c
 * @author  Haris Turkmanovic(haris@etf.rs)
 * @date    2021
 * @brief   SRV Zadatak 16
 *
 * Ported to the RVisio SoC (RISC-V, simulator) from the MSP430F5529 example SRV_2_9
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
/** "Char processing" task priority */
#define mainCHAR_PROCESSING_TASK_PRIO   ( 1 )
/** "LE Diode task" Priority */
#define mainDIODE_CONTROL_TASK_PRIO     ( 2 )

/* Queue parameter value*/
#define mainCHAR_QUEUE_LENGTH             5

static void prvSetupHardware( void );
static void vUARTISR( BaseType_t *pxHigherPriorityTaskWoken );

/* This semaphore will be used to signal "Diode Control" task to read prvDIODE_CONTROL*/
xSemaphoreHandle    xEvent_DiodeCommand;
/* This is a semaphore which will be used as mutex to prevent simultaneous
 * access to shared resource */
xSemaphoreHandle    xGuard_ControlMsg;
/* This queue will be used to buffer char received over UART interface*/
xQueueHandle        xCharQueue;
/**/
diode_command_t     prvDIODE_COMMAND;
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
        if(commandToSend != DIODE_COMMAND_UNDEF){
            xSemaphoreTake(xGuard_ControlMsg, portMAX_DELAY);
            prvDIODE_COMMAND = commandToSend;
            xSemaphoreGive(xGuard_ControlMsg);
            xSemaphoreGive(xEvent_DiodeCommand);
        }
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
        /* Wait on event*/
        xSemaphoreTake(xEvent_DiodeCommand, portMAX_DELAY);
        /* Read shared variable*/
        xSemaphoreTake(xGuard_ControlMsg, portMAX_DELAY);
        commandToProcess = prvDIODE_COMMAND;
        xSemaphoreGive(xGuard_ControlMsg);
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
    /* Create FreeRTOS objects  */
    /* Create semaphores        */
    xEvent_DiodeCommand     =   xSemaphoreCreateBinary();
    /* Create MUTEX             */
    xGuard_ControlMsg       =   xSemaphoreCreateMutex();
    /* Create Queue*/
    xCharQueue              =   xQueueCreate(mainCHAR_QUEUE_LENGTH,sizeof(char));
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
    /* Initialize UART (uart0, Console panel): interrupt when a character arrives (UCRXIE) */
    vHALInitUART( pdTRUE );
    vPortSetInterruptHandler( UART_IRQ, vUARTISR );
    /* initialize LEDs */
    vHALInitLED();
    /* Interrupts are enabled when the scheduler starts the first task */
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
