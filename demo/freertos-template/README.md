# FreeRTOS template for the RVisio SoC

Starting point for a FreeRTOS application on the simulator's default SoC. It contains everything needed to
build: the kernel, the SoC port, the settings and an application with one example task (blinking LED).

```
FreeRTOS-Kernel/   FreeRTOS-Kernel V11.3.1 (MIT) – only the files needed for building, unchanged
port/              port for the RVisio SoC (see ../freertos-demo/README.md – what was adapted and why)
src/
  main.c           YOUR APPLICATION – tasks, queues, interrupts
  FreeRTOSConfig.h kernel settings (tick 1 ms = 10 000 cycles, heap 20 KB, priorities 0–4 …)
  soc.h            SoC register addresses (details: simulator → SoC → "📄 Datasheet")
  startup.S        reset: stack, mtvec = FreeRTOS trap handler, .bss, main
  libc_min.c, libc/  memset/memcpy/memcmp/strlen (no standard library)
link.ld            Flash 0x0 (code), SRAM 0x10000 (data, heap, stacks)
Makefile           make → build/app.elf, build/app.lst, build/app.map
.vscode/           debugging from VS Code via the simulator's GDB server (like soc-demo)
```

## New project

1. Copy the whole folder and rename it (e.g. `my-project`); `.vscode/` has no absolute paths, so it works
   in the new folder as it is.
2. Write the application in `src/main.c`; add new `.c` files to `SRCS` in the `Makefile`.
3. `make` (in WSL or Linux), then in the simulator: SoC → "📂 ELF" → `build/app.elf`,
   or from VS Code: "🐞 GDB server" in the simulator, then F5.

## Rules

* Core: FemtoRV32 Intermissum-FDE, AONR-V (all three organizations) or VeeR EH2. The original FemtoRV32
  Intermissum (read-only mepc) and Quark (no interrupts) cannot run FreeRTOS.
* Peripheral interrupt: `vPortSetInterruptHandler(IRQ, handler)` – the handler is called from the interrupt, may
  use only `...FromISR` functions and must clear the device's request. Interrupts **4** (TIMER0, tick) and
  **31** (INTC SOFT software interrupt, task switch) belong to the kernel.
* The task stack is given in words (`configMINIMAL_STACK_SIZE` = 192 words = 768 B). Printing and deeper calls need
  more; a stack overflow is reported on the console (`configCHECK_FOR_STACK_OVERFLOW = 2`).
* The heap (`configTOTAL_HEAP_SIZE`) + `.data/.bss` must fit into the 64 KB SRAM – `build/app.map`.
* Output: `console_puts()` / `console_putu()` in `main.c` (UART0 = the **Console** panel in the simulator). If
  several tasks print, protect the console with a mutex (`#include "semphr.h"`, `xSemaphoreCreateMutex()`, then
  `xSemaphoreTake` / `xSemaphoreGive` around the output).
