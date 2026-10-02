/*
 * main.c – TEMPLATE for a FreeRTOS application on the RVisio SoC.
 *
 * Starting point for your own project: the kernel (FreeRTOS-Kernel V11.3.1), the SoC port (port/),
 * the settings (src/FreeRTOSConfig.h) and one example task. Add your own tasks, queues and interrupts.
 *
 *   - task:  xTaskCreate( vMyTask, "name", configMINIMAL_STACK_SIZE, NULL, priority, &handle );
 *   - delay: vTaskDelay( pdMS_TO_TICKS( 100 ) );          1 tick = 1 ms = 10 000 cycles
 *   - peripheral interrupt: vPortSetInterruptHandler( UART_IRQ, handler );   (the handler uses ...FromISR)
 *     interrupts 4 (TIMER0, tick) and 31 (software, task switch) belong to the kernel
 *   - output: console_puts / console_putu (UART0 = the Console panel in the simulator)
 */
#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "soc.h"
#include "port_soc.h"

/* ------------------------------------------------------------------ console (UART0) */

void console_putc( char c )
{
    while( UART_STATUS & 4 ) { }     /* TX FIFO full */
    UART_TXDATA = ( uint8_t ) c;
}

void console_puts( const char * s )
{
    while( *s ) console_putc( *s++ );
}

void console_putu( uint32_t v )
{
    char buf[ 11 ];
    int n = 0;
    do { buf[ n++ ] = ( char ) ( '0' + v % 10 ); v /= 10; } while( v );
    while( n ) console_putc( buf[ --n ] );
}

/* ------------------------------------------------------------------ tasks */

/* Example: LED 0 toggles every 100 ms. Replace with your own tasks. */
static void vBlinkTask( void * pv )
{
    ( void ) pv;
    for( ; ; )
    {
        GPIO_OUT ^= 1;
        vTaskDelay( pdMS_TO_TICKS( 100 ) );
    }
}

/* ------------------------------------------------------------------ main */

int main( void )
{
    console_puts( "FreeRTOS " tskKERNEL_VERSION_NUMBER " - application\n" );

    xTaskCreate( vBlinkTask, "blink", configMINIMAL_STACK_SIZE, NULL, 1, NULL );

    vTaskStartScheduler();           /* does not return */
    for( ; ; ) { }
}

/* ------------------------------------------------------------------ hooks and errors (required by the kernel) */

static void prvHalt( const char * pcWhat, const char * pcInfo, unsigned long ulLine )
{
    taskDISABLE_INTERRUPTS();
    console_puts( "\n*** " ); console_puts( pcWhat ); console_puts( pcInfo );
    if( ulLine ) { console_putc( ':' ); console_putu( ulLine ); }
    console_putc( '\n' );
    while( UART_STATUS & 1 ) { }     /* wait for the UART to send the whole message */
    __asm volatile ( "ebreak" );     /* halt the processor – the debugger shows where */
    for( ; ; ) { }
}

void vAssertCalled( const char * pcFile, unsigned long ulLine ) { prvHalt( "configASSERT: ", pcFile, ulLine ); }
void vApplicationMallocFailedHook( void ) { prvHalt( "out of memory (heap)", "", 0 ); }
void vApplicationStackOverflowHook( TaskHandle_t xTask, char * pcTaskName ) { ( void ) xTask; prvHalt( "stack overflow: ", pcTaskName, 0 ); }
