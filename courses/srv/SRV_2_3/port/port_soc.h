/* port_soc.h – application interrupts in the FreeRTOS port for the RVisio SoC. */
#ifndef PORT_SOC_H
#define PORT_SOC_H

#include <stdint.h>
#include "FreeRTOS.h"

/* Application interrupt handler: runs in interrupt context (only the ...FromISR FreeRTOS functions). If it
 * wakes a higher-priority task, set *pxHigherPriorityTaskWoken = pdTRUE – the task switch happens
 * on exit from the interrupt. The device must clear its request (otherwise the interrupt repeats). */
typedef void ( *PortIrqHandler_t )( BaseType_t * pxHigherPriorityTaskWoken );

/* Registers a handler for interrupt number ulIrq (INTC) and enables it; NULL disables it.
 * Interrupts 4 (TIMER0, tick) and 31 (software, task switch) belong to the kernel. */
void vPortSetInterruptHandler( uint32_t ulIrq, PortIrqHandler_t pxHandler );

extern volatile uint32_t ulPortYields;

#endif
