/**
 * @file    main.c
 * @author  Haris Turkmanovic(haris@etf.rs)
 * @date    2021
 * @brief   SRV Zadatak 23
 *
 * Ported to the RVisio SoC (RISC-V, simulator) from the MSP430F5529 example SRV_2_16
 * (ETF5529 board). Hardware access goes through hal/ (same names as ETF5529_HAL).
 */

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "event_groups.h"

/* Hardware includes. */
#include "port_soc.h"

/* User's includes */
#include "hal_ETF_5529.h"

#define ULONG_MAX                       0xFFFFFFFF

/** "Display task" priority */
#define mainDISPLAY_TASK_PRIO           ( 1 )
/** "ADC task" priority */
#define mainADC_TASK_PRIO               ( 2 )
/** "Button task" priority */
#define mainBUTTON_TASK_PRIO            ( 3 )
/** "Diode task" priority */
#define mainDIODE_TASK_PRIO             ( 3 )

/** ADC Task bit masks */
#define mainADC_TAKE_SAMPLE             0x01    /* Start AD conversion bit mask */
#define mainADC_CHANGE_CHANEL           0x02    /* Change AD conversion channel bit mask */

/* Display queue parameters value*/
/* Queue with length 1 is mailbox*/
#define mainDISPLAY_QUEUE_LENGTH            1

static void prvSetupHardware( void );
static void vPORT1ISR( BaseType_t *pxHigherPriorityTaskWoken );
static void vADC12ISR( BaseType_t *pxHigherPriorityTaskWoken );

/* This queue will be used to send data to display task*/
xQueueHandle        xDisplayMailbox;
/* This handle will be used as Button task instance*/
TaskHandle_t        xButtonTaskHandle;
/* This handle will be used as ADC task instance*/
TaskHandle_t        xADCTaskHandle;
/* This handle will be used as DIODE task instance*/
TaskHandle_t        xDIODETaskHandle;

/**
 * @brief "Diode Control" task function
 *
 * This task set diode state based on prvDIODE_COMMAND variable
 */
static void prvDiodeControlTaskFunction( void *pvParameters )
{
    uint8_t         diodeToTurnOn   =   LED3;
    uint8_t         diodeToTurnOff  =   LED4;
    halSET_LED(diodeToTurnOn);
    halCLR_LED(diodeToTurnOff);
    for ( ;; )
    {
        /* Wait for notification*/
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        diodeToTurnOn = diodeToTurnOn == LED3? LED4 : LED3;
        diodeToTurnOff = diodeToTurnOn == LED3? LED4 : LED3;
        halSET_LED(diodeToTurnOn);
        halCLR_LED(diodeToTurnOff);
    }
}
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
 * This task start ADC conversion or change ADC channel wich depends on
 * notification value
 */
static void prvADCTaskFunction( void *pvParameters )
{
    uint32_t    notifyValue;
    uint8_t     channel      = 1;
    for ( ;; )
    {
       /* Waits for notification value */
       xTaskNotifyWait(ULONG_MAX, ULONG_MAX, &notifyValue, portMAX_DELAY);
       /* Check if notification bit value, defined with mainADC_TAKE_SAMPLE mask, is set*/
       if((notifyValue & mainADC_TAKE_SAMPLE) != 0){
           halADC_START();
       }
       /* Check if notification bit value, defined with mainADC_CHANGE_CHANEL mask, is set*/
       if((notifyValue & mainADC_CHANGE_CHANEL) != 0){
           /* Determine what is next channel */
           channel        = channel == 1 ? 0 : 1;
           /* Change ADC periphery channel */
           switch(channel){
           case 0:
               halADC_SELECT_CHANNEL(0);
               xTaskNotifyGive(xDIODETaskHandle);
               break;
           case 1:
               halADC_SELECT_CHANNEL(1);
               xTaskNotifyGive(xDIODETaskHandle);
               break;
           }
       }
    }
}
/**
 * @brief "Button Task" Function
 *
 * This task waits for ISR notification
 */
static void prvButtonTaskFunction( void *pvParameters )
{
    volatile uint16_t i;
    /*Initial button states are 1 because of pull-up configuration*/
    uint8_t         currentButtonState  = 1;
    for ( ;; )
    {
        /* Wait for notification from ISR*/
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        /*wait for a little to check that button is still pressed*/
        for(i = 0; i < 1000; i++);
        /* check if button SW3 is pressed*/
        currentButtonState = halREAD_BUTTON( HAL_BUTTON_S3 );
        if(currentButtonState == 0){
            /* If S3 is pressed set ADC task notification value bit defined
             * with mainADC_TAKE_SAMPLE mask */
            xTaskNotify(xADCTaskHandle, mainADC_TAKE_SAMPLE, eSetBits);
            continue;
        }
        /* check if button S4 is pressed*/
        currentButtonState = halREAD_BUTTON( HAL_BUTTON_S4 );
        if(currentButtonState == 0){
            /* If S4 is pressed set ADC task notification value bit defined
             * with mainADC_CHANGE_CHANEL mask */
            xTaskNotify(xADCTaskHandle, mainADC_CHANGE_CHANEL, eSetBits);
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
                 &xADCTaskHandle
               );
    xTaskCreate( prvButtonTaskFunction,
                 "Button Task",
                 configMINIMAL_STACK_SIZE,
                 NULL,
                 mainBUTTON_TASK_PRIO,
                 &xButtonTaskHandle
               );
    xTaskCreate( prvDiodeControlTaskFunction,
                 "Diode Task",
                 configMINIMAL_STACK_SIZE,
                 NULL,
                 mainBUTTON_TASK_PRIO,
                 &xDIODETaskHandle
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
    /* Inputs; interrupt when pressed for: S3, S4 (P1IE/P1IES on the board) */
    vHALInitButtons( HAL_BUTTON_S3 | HAL_BUTTON_S4, HAL_BUTTON_S3 | HAL_BUTTON_S4 );
    /* Button interrupt handler (PORT1_VECTOR on the board) */
    vPortSetInterruptHandler( GPIO_IRQ, vPORT1ISR );
    /*Initialize ADC: on, interrupt at the end of a conversion, input 1 */
    vHALInitADC( 1 );
    vPortSetInterruptHandler( ADC_IRQ, vADC12ISR );
    /* initialize LEDs */
    vHALInitLED();
    /* initialize display*/
    vHAL7SEGInit();
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
    /* Notify Button task that one of the button is pressed*/
    /* Note: This check is not truly necessary but it is good to
     * have it*/
    if(((ulIFG & HAL_BUTTON_S3) == HAL_BUTTON_S3) || ((ulIFG & HAL_BUTTON_S4) == HAL_BUTTON_S4)){
        vTaskNotifyGiveFromISR(xButtonTaskHandle, pxHigherPriorityTaskWoken);
    }
    /*Clear IFG register on exit*/
    halBUTTON_CLEAR_IFG(HAL_BUTTON_S3 | HAL_BUTTON_S4);
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
