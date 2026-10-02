/* FreeRTOSConfig.h – FreeRTOS on the RVisio SoC. */
#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* "CPU clock" = simulator cycle. A tick every 10 000 cycles: enough for even the slowest core
 * (FemtoRV32 with a small cache) to handle the tick and a task switch, while many ticks are still visible in the simulator. */
#define configCPU_CLOCK_HZ                       ( 10000000UL )
#define configTICK_RATE_HZ                       ( ( TickType_t ) 1000 )   /* 1 tick = 1 ms = 10 000 cycles */

/* The SoC has no mtime/mtimecmp (CLINT): the tick comes from TIMER0 via the INTC (port/port_soc.c). */
#define configMTIME_BASE_ADDRESS                 ( 0 )
#define configMTIMECMP_BASE_ADDRESS              ( 0 )
#define configISR_STACK_SIZE_WORDS               ( 256 )   /* separate stack for interrupts (1 KB) */

#define configUSE_PREEMPTION                     1
#define configUSE_TIME_SLICING                   1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION  0          /* no Zbb (clz) – generic selection */
#define configMAX_PRIORITIES                     ( 5 )
#define configMINIMAL_STACK_SIZE                 ( ( unsigned short ) 192 )   /* in words */
#define configMAX_TASK_NAME_LEN                  ( 12 )
#define configTICK_TYPE_WIDTH_IN_BITS            TICK_TYPE_WIDTH_32_BITS
#define configIDLE_SHOULD_YIELD                  1
#define configUSE_TASK_NOTIFICATIONS             1
#define configUSE_MUTEXES                        1
#define configUSE_RECURSIVE_MUTEXES              0
#define configUSE_COUNTING_SEMAPHORES            1
#define configQUEUE_REGISTRY_SIZE                0
#define configUSE_QUEUE_SETS                     0
#define configUSE_NEWLIB_REENTRANT               0
#define configENABLE_BACKWARD_COMPATIBILITY      0

/* memory: dynamic allocation only, heap_4 in SRAM */
#define configSUPPORT_STATIC_ALLOCATION          0
#define configSUPPORT_DYNAMIC_ALLOCATION         1
#define configTOTAL_HEAP_SIZE                    ( ( size_t ) ( 20 * 1024 ) )
#define configAPPLICATION_ALLOCATED_HEAP         0

/* hooks and checks */
#define configUSE_IDLE_HOOK                      0
#define configUSE_TICK_HOOK                      0
#define configUSE_MALLOC_FAILED_HOOK             1
/* stack overflow check on every task switch: 2 = stack top + 16 B of the 0xA5 pattern at the bottom,
 * 0 = no check ("make OVERFLOW_CHECK=0" – shows what happens when nobody is watching) */
#ifndef configCHECK_FOR_STACK_OVERFLOW
    #define configCHECK_FOR_STACK_OVERFLOW       2
#endif
#define configUSE_TRACE_FACILITY                 1
#define configUSE_STATS_FORMATTING_FUNCTIONS     0
#define configGENERATE_RUN_TIME_STATS            0

/* software timers */
#define configUSE_TIMERS                         1
#define configTIMER_TASK_PRIORITY                ( configMAX_PRIORITIES - 1 )
#define configTIMER_QUEUE_LENGTH                 4
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

/* configASSERT: print to UART and halt the processor (ebreak) – see main.c */
void vAssertCalled( const char * pcFile, unsigned long ulLine );
#define configASSERT( x )    if( ( x ) == 0 ) vAssertCalled( __FILE__, __LINE__ )

#endif /* FREERTOS_CONFIG_H */
