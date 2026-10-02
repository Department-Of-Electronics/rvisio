/*
 * FreeRTOS port for the RVisio SoC – the SoC-specific part.
 *
 * The official RISC-V port (port.c, portASM.S, portContext.h – copied unchanged from
 * FreeRTOS-Kernel/portable/GCC/RISC-V) saves and restores the task context. This file holds what depends on the SoC:
 *
 *  1) Tick: the SoC has no mtime/mtimecmp (CLINT), so the tick comes from TIMER0 (periodic, interrupt 4) via the INTC.
 *  2) Task switch (portYIELD): the cores in the simulator have no exceptions – ecall/ebreak halt
 *     the processor – so the interrupt controller's software interrupt (INTC SOFT, interrupt 31) is used, like PendSV
 *     on ARM Cortex-M. If interrupts are enabled, vPortYield waits for the interrupt to be handled; if not
 *     (critical section), the switch happens as soon as interrupts are enabled again.
 *  3) All interrupts (regardless of mcause, which differs per core: FemtoRV 0x80000000, EH2
 *     0x80000007, AONR-V 0x8000000B) go to freertos_risc_v_application_interrupt_handler(), which asks
 *     the INTC (CLAIM) who is requesting the interrupt: the tick, the software interrupt or an application interrupt (handler table).
 *  4) FemtoRV32: mret does not restore MIE from MPIE (the interrupt handler is locked via mcause, MIE stays 1).
 *     A new task has mstatus with MIE = 0 in its frame (the port expects mret to set it), so on such a
 *     core MIE is written into the frame of the task that is about to get the processor. The core type is detected at startup.
 */
#include "FreeRTOS.h"
#include "task.h"
#include "soc.h"
#include "port_soc.h"

#define MSTATUS_MIE   0x08u
#define MSTATUS_MPIE  0x80u

static PortIrqHandler_t xHandlers[ 32 ];
static BaseType_t xMretRestoresMie = pdTRUE;   /* standard behaviour (AONR-V, VeeR EH2) */
volatile uint32_t ulPortYields = 0;             /* number of software interrupts (for statistics) */

extern void * volatile pxCurrentTCB;            /* tasks.c; the first TCB member is the top of the task stack */

static inline uint32_t prvReadMstatus( void )
{
    uint32_t v;
    __asm volatile ( "csrr %0, mstatus" : "=r" ( v ) );
    return v;
}

/* Called by xPortStartScheduler() (interrupts are disabled at that point). */
void vPortSetupTimerInterrupt( void )
{
    /* Does mret restore MIE from MPIE? A core without the MPIE bit (FemtoRV32) reads it as 0. */
    const uint32_t ulSaved = prvReadMstatus();
    __asm volatile ( "csrw mstatus, %0" :: "r" ( MSTATUS_MPIE ) );
    xMretRestoresMie = ( prvReadMstatus() & MSTATUS_MPIE ) ? pdTRUE : pdFALSE;
    __asm volatile ( "csrw mstatus, %0" :: "r" ( ulSaved & ~MSTATUS_MIE ) );

    /* A task switch requires writing mepc (mret returns into the task that gets the processor). The original
     * FemtoRV32 Intermissum has a read-only mepc – a preemptive RTOS cannot run on it. */
    uint32_t ulMepc;
    __asm volatile ( "csrw mepc, %1 \n csrr %0, mepc" : "=r" ( ulMepc ) : "r" ( 0x100u ) );
    if( ulMepc != 0x100u )
    {
        vAssertCalled( "port_soc.c: the core does not allow writing mepc (original FemtoRV32 Intermissum) - "
                       "choose FemtoRV32 Intermissum-FDE, AONR-V or VeeR EH2", __LINE__ );
    }

    TIMER_CTRL = 0;
    TIMER_STATUS = 1;
    TIMER_LOAD = configCPU_CLOCK_HZ / configTICK_RATE_HZ;   /* cycles per tick */
    INTC_SOFT = 0;
    INTC_ENABLE |= ( 1u << TIMER_IRQ ) | ( 1u << SOFT_IRQ );
    TIMER_CTRL = 7;                                          /* enabled, periodic, with interrupt */
}

void vPortYield( void )
{
    INTC_SOFT = 1;
    if( prvReadMstatus() & MSTATUS_MIE )
    {
        while( INTC_SOFT != 0 )
        {
            /* the interrupt arrives within a few cycles; the handler clears the request */
        }
    }
}

void vPortSetInterruptHandler( uint32_t ulIrq, PortIrqHandler_t pxHandler )
{
    configASSERT( ulIrq < 32 && ulIrq != TIMER_IRQ && ulIrq != SOFT_IRQ );
    taskENTER_CRITICAL();
    xHandlers[ ulIrq ] = pxHandler;
    if( pxHandler != NULL ) INTC_ENABLE |= 1u << ulIrq;
    else INTC_ENABLE &= ~( 1u << ulIrq );
    taskEXIT_CRITICAL();
}

/* Called by the trap handler in portASM.S for every interrupt, on the ISR stack, after saving the context
 * of the interrupted task; after returning, it restores the context of the task pxCurrentTCB points to. */
void freertos_risc_v_application_interrupt_handler( void )
{
    BaseType_t xSwitch = pdFALSE;
    uint32_t ulClaim;

    while( ( ulClaim = INTC_CLAIM ) != 0 )
    {
        const uint32_t ulIrq = ulClaim - 1;

        if( ulIrq == TIMER_IRQ )
        {
            TIMER_STATUS = 1;
            if( xTaskIncrementTick() != pdFALSE ) xSwitch = pdTRUE;
        }
        else if( ulIrq == SOFT_IRQ )
        {
            INTC_SOFT = 0;
            ulPortYields++;
            xSwitch = pdTRUE;
        }
        else if( xHandlers[ ulIrq ] != NULL )
        {
            xHandlers[ ulIrq ]( &xSwitch );
        }
        else
        {
            INTC_ENABLE &= ~( 1u << ulIrq );   /* nobody handles it – disable it */
        }
    }

    if( xSwitch != pdFALSE )
    {
        vTaskSwitchContext();
    }

    if( xMretRestoresMie == pdFALSE )
    {
        /* frame: [0] = mepc, [1] = mstatus (portContext.h) */
        StackType_t * pxTop = *( StackType_t ** ) pxCurrentTCB;
        pxTop[ 1 ] |= MSTATUS_MIE;
    }
}
