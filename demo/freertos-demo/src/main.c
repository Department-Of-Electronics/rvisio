/*
 * main.c – FreeRTOS on the RVisio SoC: producer and consumer.
 *
 *   button (GPIO, interrupt 3) ──► interrupt handler ──(notification)──► PRODUCER ──(queue)──► CONSUMER ──► console (UART0)
 *
 *   interrupt handler  sees which input went 0 → 1 (press) and wakes the producer with a notification
 *   producer           (priority 2) waits for a notification, builds a message for every pressed button and sends it to the queue
 *   consumer           (priority 1) waits for a message in the queue and prints it to the console; the LEDs show the number of presses
 *
 * In the simulator: Peripherals panel → gpio0 → click a button (0–7); the output is in the Console panel.
 * On a real board the program would work the same: the tasks sleep (the processor runs the idle task) until a button is pressed.
 */
#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "soc.h"
#include "port_soc.h"

typedef struct
{
    uint32_t ulPin;       /* which button */
    uint32_t ulCount;     /* how many times it was pressed */
    TickType_t xTick;     /* when (ms since start) */
} ButtonMsg_t;

static QueueHandle_t xMessages;      /* producer -> consumer */
static TaskHandle_t xProducer;

/* ------------------------------------------------------------------ console (UART0) */

static void uart_putc( char c )
{
    while( UART_STATUS & 4 ) { }     /* TX FIFO full */
    UART_TXDATA = ( uint8_t ) c;
}

static void uart_puts( const char * s )
{
    while( *s ) uart_putc( *s++ );
}

static void uart_putu( uint32_t v )
{
    char buf[ 11 ];
    int n = 0;
    do { buf[ n++ ] = ( char ) ( '0' + v % 10 ); v /= 10; } while( v );
    while( n ) uart_putc( buf[ --n ] );
}

/* ------------------------------------------------------------------ button interrupt */

static void vGpioIrq( BaseType_t * pxWoken )
{
    const uint32_t ulChanged = GPIO_IRQ_STATUS;
    GPIO_IRQ_STATUS = ulChanged;                     /* acknowledge the interrupt */
    const uint32_t ulPressed = ulChanged & GPIO_IN;  /* change to 1 = press (release is not counted) */
    if( ulPressed != 0 )
    {
        xTaskNotifyFromISR( xProducer, ulPressed, eSetBits, pxWoken );
    }
}

/* ------------------------------------------------------------------ tasks */

static void vProducerTask( void * pv )
{
    ( void ) pv;
    uint32_t ulCounts[ 8 ] = { 0 };
    uint32_t ulPins;
    for( ; ; )
    {
        xTaskNotifyWait( 0, 0xFFFFFFFFu, &ulPins, portMAX_DELAY );   /* sleeps until a button is pressed */
        for( uint32_t pin = 0; pin < 8; pin++ )
        {
            if( ulPins & ( 1u << pin ) )
            {
                ButtonMsg_t xMsg = { pin, ++ulCounts[ pin ], xTaskGetTickCount() };
                xQueueSend( xMessages, &xMsg, 0 );                  /* queue full -> the message is dropped */
            }
        }
    }
}

static void vConsumerTask( void * pv )
{
    ( void ) pv;
    ButtonMsg_t xMsg;
    uint32_t ulTotal = 0;
    for( ; ; )
    {
        xQueueReceive( xMessages, &xMsg, portMAX_DELAY );          /* sleeps until a message arrives */
        ulTotal++;
        GPIO_OUT = ulTotal;                                         /* LED: total number of presses */
        uart_putu( xMsg.xTick );
        uart_puts( " ms  button " );
        uart_putu( xMsg.ulPin );
        uart_puts( " pressed (" );
        uart_putu( xMsg.ulCount );
        uart_puts( ". time)\n" );
    }
}

/* ------------------------------------------------------------------ main */

int main( void )
{
    uart_puts( "FreeRTOS " tskKERNEL_VERSION_NUMBER " - producer/consumer\n" );
    uart_puts( "press a button on the GPIO (Peripherals panel, gpio0)\n" );

    xMessages = xQueueCreate( 8, sizeof( ButtonMsg_t ) );
    configASSERT( xMessages );
    xTaskCreate( vProducerTask, "producer", configMINIMAL_STACK_SIZE, NULL, 2, &xProducer );
    xTaskCreate( vConsumerTask, "consumer", configMINIMAL_STACK_SIZE + 64, NULL, 1, NULL );

    GPIO_IRQ_STATUS = 0xFF;
    GPIO_IRQ_EN = 0xFF;                               /* interrupt on change of all 8 inputs */
    vPortSetInterruptHandler( GPIO_IRQ, vGpioIrq );

    vTaskStartScheduler();                            /* does not return */
    for( ; ; ) { }
}

/* ------------------------------------------------------------------ hooks and errors */

static void prvHalt( const char * pcWhat, const char * pcInfo, unsigned long ulLine )
{
    taskDISABLE_INTERRUPTS();
    uart_puts( "\n*** " ); uart_puts( pcWhat ); uart_puts( pcInfo );
    if( ulLine ) { uart_putc( ':' ); uart_putu( ulLine ); }
    uart_putc( '\n' );
    while( UART_STATUS & 1 ) { }     /* wait for the UART to send the whole message */
    __asm volatile ( "ebreak" );
    for( ; ; ) { }
}

void vAssertCalled( const char * pcFile, unsigned long ulLine ) { prvHalt( "configASSERT: ", pcFile, ulLine ); }
void vApplicationMallocFailedHook( void ) { prvHalt( "out of memory (heap)", "", 0 ); }
void vApplicationStackOverflowHook( TaskHandle_t xTask, char * pcTaskName ) { ( void ) xTask; prvHalt( "stack overflow: ", pcTaskName, 0 ); }
