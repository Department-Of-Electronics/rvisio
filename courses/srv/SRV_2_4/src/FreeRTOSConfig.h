/* FreeRTOSConfig.h – examples of the course "Real Time Operating Systems" on the RVisio SoC.
 * The values follow the configuration of the MSP430F5529 examples where it makes sense (tick 1 ms,
 * 8 priorities, timers, mutexes); stack sizes and the heap are for the 32 bit RISC-V processor. */
#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* "CPU clock" = simulator cycle: a tick every 10 000 cycles */
#define configCPU_CLOCK_HZ                       ( 10000000UL )
#define configTICK_RATE_HZ                       ( ( TickType_t ) 1000 )   /* 1 tick = 1 ms = 10 000 cycles */

/* The SoC has no mtime/mtimecmp (CLINT): the tick comes from TIMER0 via the INTC (port/port_soc.c). */
#define configMTIME_BASE_ADDRESS                 ( 0 )
#define configMTIMECMP_BASE_ADDRESS              ( 0 )
#define configISR_STACK_SIZE_WORDS               ( 256 )   /* separate stack for interrupts (1 KB) */

#define configUSE_PREEMPTION                     1
#define configUSE_TIME_SLICING                   1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION  0
#define configMAX_PRIORITIES                     ( 8 )
#define configMINIMAL_STACK_SIZE                 ( ( unsigned short ) 192 )   /* in words (768 B) */
#define configMAX_TASK_NAME_LEN                  ( 24 )
#define configTICK_TYPE_WIDTH_IN_BITS            TICK_TYPE_WIDTH_32_BITS
#define configIDLE_SHOULD_YIELD                  1
#define configUSE_TASK_NOTIFICATIONS             1
#define configUSE_MUTEXES                        1
#define configUSE_RECURSIVE_MUTEXES              1
#define configUSE_COUNTING_SEMAPHORES            1
#define configQUEUE_REGISTRY_SIZE                0
#define configUSE_QUEUE_SETS                     0
#define configUSE_NEWLIB_REENTRANT               0
/* the examples use the older names too (xSemaphoreHandle, xQueueHandle) */
#define configENABLE_BACKWARD_COMPATIBILITY      1

/* memory: dynamic allocation only, heap_4 in SRAM */
#define configSUPPORT_STATIC_ALLOCATION          0
#define configSUPPORT_DYNAMIC_ALLOCATION         1
#define configTOTAL_HEAP_SIZE                    ( ( size_t ) ( 20 * 1024 ) )
#define configAPPLICATION_ALLOCATED_HEAP         0

/* hooks and checks (util.c) */
#define configUSE_IDLE_HOOK                      0
#define configUSE_TICK_HOOK                      0
#define configUSE_MALLOC_FAILED_HOOK             1
#define configCHECK_FOR_STACK_OVERFLOW           2
#define configUSE_TRACE_FACILITY                 1
#define configUSE_STATS_FORMATTING_FUNCTIONS     0
#define configGENERATE_RUN_TIME_STATS            0

/* software timers */
#define configUSE_TIMERS                         1
#define configTIMER_TASK_PRIORITY                ( 7 )
#define configTIMER_QUEUE_LENGTH                 10
#define configTIMER_TASK_STACK_DEPTH             ( configMINIMAL_STACK_SIZE )

/* which API functions are compiled */
#define INCLUDE_vTaskPrioritySet                 1
#define INCLUDE_uxTaskPriorityGet                1
#define INCLUDE_vTaskDelete                      1
#define INCLUDE_vTaskSuspend                     1
#define INCLUDE_xTaskDelayUntil                  1
#define INCLUDE_vTaskDelay                       1
#define INCLUDE_xTaskGetSchedulerState           1
#define INCLUDE_xTaskGetCurrentTaskHandle        1
#define INCLUDE_uxTaskGetStackHighWaterMark      1
#define INCLUDE_xTaskGetIdleTaskHandle           1
#define INCLUDE_xTimerPendFunctionCall           1

/* configASSERT: print to UART and halt the processor (ebreak) – see util.c */
void vAssertCalled( const char * pcFile, unsigned long ulLine );
#define configASSERT( x )    if( ( x ) == 0 ) vAssertCalled( __FILE__, __LINE__ )

#endif /* FREERTOS_CONFIG_H */
