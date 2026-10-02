/*
 * FreeRTOS port for the RVisio SoC – chip-specific extensions.
 *
 * The SoC has no CLINT and no mtime/mtimecmp: the tick comes from TIMER0 through the interrupt controller (INTC),
 * so the trap handler in portASM.S forwards every interrupt to freertos_risc_v_application_interrupt_handler()
 * (port_soc.c). The cores have no additional registers that need saving in the task context.
 */
#ifndef __FREERTOS_RISC_V_EXTENSIONS_H__
#define __FREERTOS_RISC_V_EXTENSIONS_H__

#define portasmHAS_SIFIVE_CLINT         0
#define portasmHAS_MTIME                0
#define portasmADDITIONAL_CONTEXT_SIZE  0

.macro portasmSAVE_ADDITIONAL_REGISTERS
    /* No additional registers. */
    .endm

.macro portasmRESTORE_ADDITIONAL_REGISTERS
    /* No additional registers. */
    .endm

#endif /* __FREERTOS_RISC_V_EXTENSIONS_H__ */
