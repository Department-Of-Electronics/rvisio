/**
 * @file util.c
 * @author Strahinja Jankovic (jankovics@etf.bg.ac.rs)
 * @date 2019
 * @brief Utility functions
 *
 * Hooks that are not needed in everyday work. On the RVisio SoC the tick comes from TIMER0 and is
 * set up by the port (port/port_soc.c), so vApplicationSetupTimerInterrupt() and the low power idle
 * hook of the MSP430 are not needed. When something goes wrong the hooks print the reason to the
 * console (UART0) and stop the processor (ebreak), so the simulator shows where it happened.
 */

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"

/* Hardware includes. */
#include "soc.h"

static void prvPrint( const char * s )
{
    while( *s ){
        while( UART_STATUS & 4u ) { }     /* TX FIFO full */
        UART_TXDATA = ( uint8_t ) *s++;
    }
}

static void prvHalt( const char * pcWhat, const char * pcInfo )
{
    taskDISABLE_INTERRUPTS();
    prvPrint( "\r\n*** " );
    prvPrint( pcWhat );
    prvPrint( pcInfo );
    prvPrint( "\r\n" );
    while( UART_STATUS & 1u ) { }         /* wait until the whole message is sent */
    __asm volatile ( "ebreak" );
    for( ;; );
}

/**
 * @author FreeRTOS
 * @brief Called when malloc fails
 */
void vApplicationMallocFailedHook( void )
{
    /* Called if a call to pvPortMalloc() fails because there is insufficient
    free memory available in the FreeRTOS heap.  pvPortMalloc() is called
    internally by FreeRTOS API functions that create tasks, queues or
    semaphores. */
    prvHalt( "out of memory (FreeRTOS heap)", "" );
}

/**
 * @author FreeRTOS
 * @brief Stack overflow hook
 */
void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
    ( void ) pxTask;
    /* Run time stack overflow checking is performed if
    configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
    function is called if a stack overflow is detected. */
    prvHalt( "stack overflow in task ", pcTaskName );
}

/**
 * @brief configASSERT failed
 */
void vAssertCalled( const char * pcFile, unsigned long ulLine )
{
    ( void ) ulLine;
    prvHalt( "configASSERT failed in ", pcFile );
}
