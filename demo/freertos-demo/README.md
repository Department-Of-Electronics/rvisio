# FreeRTOS on the RVisio SoC

> **Run it in the browser:** [rvisio.com](https://rvisio.com) → Open simulator → **System on chip (SoC)** →
> **📦 FreeRTOS demos** → *Producer and consumer* → **⬇ Load program** → **▶ Run** (no toolchain needed).


FreeRTOS-Kernel **V11.3.1** (latest release, https://github.com/FreeRTOS/FreeRTOS-Kernel, MIT license)
ported to the simulator's default SoC: 64 KB Flash at 0x0, 64 KB SRAM at 0x10000, UART0, GPIO,
TIMER0 and the interrupt controller (INTC). The starting point for your own application is `../freertos-template`,
and the SoC registers are described in the datasheet (simulator: SoC → "📄 Datasheet").

**Demonstration – producer and consumer:**

```
button (GPIO, interrupt 3) ─► interrupt handler ─(notification)─► PRODUCER ─(queue)─► CONSUMER ─► console (UART0)
```

* the interrupt handler sees which input went 0 → 1 (press) and wakes the producer (`xTaskNotifyFromISR`);
* the producer (priority 2) builds a message {button, press count, time} for every pressed button and sends it to the queue;
* the consumer (priority 1) waits for a message in the queue and prints it to the console; the LEDs show the total number of presses.

```
FreeRTOS-Kernel/   kernel – git clone, tag V11.3.1, NO changes
port/              port for the SoC (portable/GCC/RISC-V + changes marked with [RVisio SoC])
  port.c, portASM.S, portContext.h   copied unchanged from FreeRTOS-Kernel/portable/GCC/RISC-V
  portmacro.h                        the only change: portYIELD() = vPortYield() instead of ecall
  freertos_risc_v_chip_specific_extensions.h   no mtime/CLINT, no additional registers
  port_soc.c / port_soc.h            tick (TIMER0), task switch (INTC SOFT), interrupt dispatch
src/
  FreeRTOSConfig.h   kernel settings (tick 1 ms = 10 000 cycles, heap 20 KB, ISR stack 1 KB)
  main.c             demonstration: producer and consumer (button → queue → console)
  startup.S          stack, mtvec = freertos_risc_v_trap_handler, .bss, main
  soc.h              SoC register addresses
  libc_min.c, libc/  memset/memcpy/… (the program has no standard library)
link.ld, Makefile, .vscode/   like soc-demo (VS Code + the simulator's GDB server)
```

## Building and running

```bash
make            # in WSL: build/app.elf, build/app.lst, build/app.map
```

In the simulator: analysis **System on chip (SoC)**, default configuration, core FemtoRV32
Intermissum-FDE, AONR-V (all three organizations) or VeeR EH2 → "📂 ELF" →
`build/app.elf` → "▶ Run". Press buttons in the **Sensors and actuators** panel (hold = press);
messages appear in the **Console** panel, and the LEDs on the GPIO count the presses.
From VS Code: "🐞 GDB server" in the simulator, then F5 (as with soc-demo).

Example output (buttons 2, 5, 2 and 6 pressed):

```
FreeRTOS V11.3.1 - producer/consumer
press a button (Sensors and actuators panel, sw0-sw7)
30 ms  button 2 pressed (1. time)
60 ms  button 5 pressed (1. time)
90 ms  button 2 pressed (2. time)
91 ms  button 6 pressed (1. time)
```

The previous version of the demonstration (producer every 3 ms, mutex, software timer, UART echo and statistics
at the end) shows the cost of an RTOS on a slow processor well; the same API is used in the template.

## What the port needed

The official RISC-V port assumes two things the SoC does not have:

1. **mtime/mtimecmp (CLINT) for the tick.** Here the tick comes from **TIMER0** (periodic, `LOAD = 10 000` cycles,
   interrupt 4) via the INTC. `configMTIME_BASE_ADDRESS = 0`, so the trap handler in `portASM.S` sends every interrupt
   to `freertos_risc_v_application_interrupt_handler()` (`port_soc.c`), which reads `INTC_CLAIM`:
   tick → `xTaskIncrementTick()`, software interrupt → task switch, anything else → the application handler
   (`vPortSetInterruptHandler(irq, handler)`).
2. **`ecall` for the task switch (`portYIELD`).** The cores in the simulator have no exceptions (ecall/ebreak
   halt the processor), so `portYIELD()` writes 1 to **INTC SOFT** (0x00805000 + 0x0C) – software
   interrupt number 31, the same as `msip` in the CLINT or PendSV on ARM Cortex-M. If interrupts are enabled,
   `vPortYield()` waits for the handler to process it; in a critical section the switch happens as soon as interrupts are enabled.

Differences between the cores that the port covers:

| core | mcause on interrupt | mret | write to mepc | FreeRTOS |
|---|---|---|---|---|
| FemtoRV32 Intermissum-FDE | 0x80000000 | does not restore MIE (locking via mcause) | yes (added in FDE) | ✔ |
| FemtoRV32 Intermissum (original) | 0x80000000 | same | **no** | ✘ – the port reports it at startup |
| AONR-V (all three organizations) | 0x8000000B | standard (MIE ← MPIE) | yes | ✔ |
| VeeR EH2 | 0x80000007 | standard | yes | ✔ |
| FemtoRV32 Quark | no interrupts | – | – | ✘ |

* The trap handler distinguishes an interrupt from an exception by bit 31 (`mcause < 0`), so the differences in `mcause` do not matter.
* FemtoRV32 does not clear MIE on interrupt entry and `mret` does not restore it from MPIE, while a new task has
  `MIE = 0` in its initial frame. At startup the port checks whether the core has the MPIE bit and, if not, writes
  `MIE = 1` into the frame of the task that gets the processor.
* A task switch means that `mret` returns into a DIFFERENT task – writing `mepc` must be possible. The original
  FemtoRV32 does not support it, so it was added to the simulator's FDE variant (`rtl/cores/femtorv32/femtorv32_intermissum_fde.v`).

## What you see in the simulator

* **Processor / Registers panel:** `mepc`, `mcause`, the jump to `freertos_risc_v_trap_handler` (0xA00).
* **Stack analyzer:** every task has its own stack in the heap (`ucHeap`); the interrupt handler switches to
  a separate ISR stack (`xISRStack`). On a task switch `sp` jumps between stacks.
* **Peripheral events and interrupts:** "software interrupt (31) requested" on every `portYIELD`, the timer tick.
* While nobody presses a button, both tasks sleep and the processor runs the idle task; the only interrupts are the tick
  (TIMER0, every 10 000 cycles). A button press: GPIO interrupt → notification → producer → queue →
  consumer – the "Events" also show the software interrupt (31) that switches the task.

## For your own program

* Tasks, queues, semaphores, timers – the standard FreeRTOS API (`FreeRTOS-Kernel/include`).
* Peripheral interrupt: `vPortSetInterruptHandler(UART_IRQ, handler)`; the handler uses only `...FromISR`
  functions and sets `*pxHigherPriorityTaskWoken`. Interrupts 4 (TIMER0) and 31 (SOFT) belong to the kernel.
* Memory: `configTOTAL_HEAP_SIZE` (20 KB) + ISR stack + `.data/.bss` must fit into SRAM (64 KB);
  `build/app.map` shows the layout. On a stack overflow or when the heap runs out, the program prints a message
  to the UART and halts the processor (`ebreak`).
