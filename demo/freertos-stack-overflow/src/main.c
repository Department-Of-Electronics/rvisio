/*
 * main.c – FreeRTOS on the RVisio SoC: STACK OVERFLOW.
 *
 *   "counter" (priority 1, stack 192 words) – created FIRST, so its stack and TCB in the heap lie right
 *             BELOW the stack of task "deep". At the top of its stack it keeps 8 "canaries" (known values)
 *             and constantly checks whether they are intact.
 *   "deep"    (priority 2, stack 192 words) – every 10 ms it recurses one level deeper (each
 *             level uses a frame with a local array on the stack) and prints how much stack is still left
 *             (uxTaskGetStackHighWaterMark). The stack grows DOWNWARD – after a few steps it passes its bottom and
 *             writes over the memory below: the heap block header, the TCB and the stack of task "counter".
 *
 *   make                    configCHECK_FOR_STACK_OVERFLOW = 2: on a task switch the kernel sees that the
 *                           bottom of the stack was overwritten → vApplicationStackOverflowHook → message and halt
 *   make OVERFLOW_CHECK=0   no check: shows what an overflow really does – the canaries of task
 *                           "counter" (or its TCB) are overwritten, and the system crashes or misbehaves
 *
 * In the simulator: SoC → load build/app.elf → "⏭ To end"; output in the Console panel, while the "Stack
 * analyzer" and "Stacks in memory" panels (Advanced layout) show how the stack of "deep" grows and crosses into
 * the memory of task "counter" (red).
 */
#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "soc.h"
#include "port_soc.h"

#define STACK_WORDS   192                 /* 768 B per task */
#define CANARY        0xC0FFEE00u

static TaskHandle_t xCounter, xDeep;
static volatile uint32_t ulCount;

/* ------------------------------------------------------------------ console (UART0) */

static void console_putc( char c )
{
    while( UART_STATUS & 4 ) { }
    UART_TXDATA = ( uint8_t ) c;
}

static void console_puts( const char * s )
{
    while( *s ) console_putc( *s++ );
}

static void console_putu( uint32_t v )
{
    char buf[ 11 ];
    int n = 0;
    do { buf[ n++ ] = ( char ) ( '0' + v % 10 ); v /= 10; } while( v );
    while( n ) console_putc( buf[ --n ] );
}

static void console_puthex( uint32_t v )
{
    console_puts( "0x" );
    for( int i = 28; i >= 0; i -= 4 ) console_putc( "0123456789abcdef"[ ( v >> i ) & 0xF ] );
}

/* ------------------------------------------------------------------ task "counter" – the victim */

static void vCounterTask( void * pv )
{
    ( void ) pv;
    volatile uint32_t ulCanary[ 8 ];      /* at the top of this task's stack */
    for( int i = 0; i < 8; i++ ) ulCanary[ i ] = CANARY + i;
    for( ; ; )
    {
        ulCount++;
        for( int i = 0; i < 8; i++ )
        {
            if( ulCanary[ i ] != CANARY + i )
            {
                taskENTER_CRITICAL();
                console_puts( "\n!!! counter: canary " ); console_putu( i );
                console_puts( " at " ); console_puthex( ( uint32_t ) &ulCanary[ i ] );
                console_puts( " was overwritten: " ); console_puthex( ulCanary[ i ] );
                console_puts( " (the stack of task \"deep\" crossed into the stack of task \"counter\")\n" );
                while( UART_STATUS & 1 ) { }
                __asm volatile ( "ebreak" );
            }
        }
    }
}

/* ------------------------------------------------------------------ task "deep" – overflows */

/* each recursion level keeps a 12-word array on the stack (+ return address and saved registers) */
static uint32_t prvRecurse( uint32_t n )
{
    volatile uint32_t buf[ 12 ];
    for( int i = 0; i < 12; i++ ) buf[ i ] = 0xDEAD0000u + n;   /* write into its own frame */
    return n ? prvRecurse( n - 1 ) + buf[ n % 12 ] : 0;
}

static void vDeepTask( void * pv )
{
    ( void ) pv;
    TickType_t xLast = xTaskGetTickCount();
    for( uint32_t depth = 1; ; depth++ )
    {
        uint32_t sp;
        __asm volatile ( "mv %0, sp" : "=r" ( sp ) );
        ( void ) prvRecurse( depth );
        console_putu( xTaskGetTickCount() );
        console_puts( " ms  depth " ); console_putu( depth );
        console_puts( ": sp " ); console_puthex( sp );
        console_puts( ", minimum free stack " ); console_putu( uxTaskGetStackHighWaterMark( NULL ) * 4 );
        console_puts( " B (counter: " ); console_putu( ulCount ); console_puts( ")\n" );
        xTaskDelayUntil( &xLast, pdMS_TO_TICKS( 10 ) );
    }
}

/* ------------------------------------------------------------------ main */

int main( void )
{
    console_puts( "FreeRTOS " tskKERNEL_VERSION_NUMBER " - stack overflow (check: " );
    console_puts( configCHECK_FOR_STACK_OVERFLOW ? "enabled" : "DISABLED" );
    console_puts( ")\n" );

    /* the order matters: the stack of "counter" is in the heap below the stack of "deep" (the stack grows downward) */
    xTaskCreate( vCounterTask, "counter", STACK_WORDS, NULL, 1, &xCounter );
    xTaskCreate( vDeepTask, "deep", STACK_WORDS, NULL, 2, &xDeep );

    vTaskStartScheduler();
    for( ; ; ) { }
}

/* ------------------------------------------------------------------ hooks and errors */

static void prvHalt( const char * pcWhat, const char * pcInfo, unsigned long ulLine )
{
    taskDISABLE_INTERRUPTS();
    console_puts( "\n*** " ); console_puts( pcWhat ); console_puts( pcInfo );
    if( ulLine ) { console_putc( ':' ); console_putu( ulLine ); }
    console_putc( '\n' );
    while( UART_STATUS & 1 ) { }
    __asm volatile ( "ebreak" );
    for( ; ; ) { }
}

void vAssertCalled( const char * pcFile, unsigned long ulLine ) { prvHalt( "configASSERT: ", pcFile, ulLine ); }
void vApplicationMallocFailedHook( void ) { prvHalt( "out of memory (heap)", "", 0 ); }

/* configCHECK_FOR_STACK_OVERFLOW = 2: on a task switch the kernel saw that the bottom of the stack was overwritten */
void vApplicationStackOverflowHook( TaskHandle_t xTask, char * pcTaskName )
{
    ( void ) xTask;
    prvHalt( "STACK OVERFLOW in task: ", pcTaskName, 0 );
}
