# SRV 2.11: ADC, mailbox and display

> **Run it in the browser:** [rvisio.com](https://rvisio.com) → Open simulator → **System on chip (SoC)** →
> **📦 FreeRTOS demos** → *🎓 Course: Real Time Operating Systems* → *SRV 2.11* → **⬇ Load program** → **▶ Run**.

Example for task 18 of the course *Real Time Operating Systems* (School of Electrical Engineering,
University of Belgrade), ported from the ETF5529 board (MSP430F5529) to the RVisio SoC.

Every 200 ms a task starts an AD conversion. The end of conversion interrupt sends the result, scaled to 0..63, through a mailbox (a queue of length 1) to the display task, which multiplexes the two digits.

**Try it:** Move the **potentiometer** slider.

**Watch in RVisio:**

* Sensors and actuators: the potentiometer and the display
* Peripherals: adc0 (conversions, result)

**FreeRTOS:** xQueueCreate(1, ...), xQueueSendToBackFromISR, xQueueReceive with timeout 0

## Build and debug

```bash
make                  # build/app.elf, build/app.lst (disassembly), build/app.map
```

Load `build/app.elf` in RVisio (SoC → **📂 ELF**), or debug it from VS Code: open this folder, click
**🐞 GDB server** in RVisio and press **F5** (see the [main README](../../../README.md#4-debug-from-vs-code)).

## What changed from the MSP430 version

The tasks, priorities, names and comments are the same as in the original `main.c`. Only the hardware access
changed: register accesses go through `hal/` (same names as `ETF5529_HAL`), `prvSetupHardware()` sets up the
RVisio peripherals and the interrupt routines are registered with `vPortSetInterruptHandler()`
(see [../README.md](../README.md)). Program texts are in English.

```
src/main.c            the example
src/util.c            FreeRTOS hooks (stack overflow, out of memory, configASSERT)
src/FreeRTOSConfig.h  kernel settings (tick 1 ms, 8 priorities, timers, heap 20 KB)
src/soc.h             RVisio SoC registers
hal/                  ETF5529 board HAL for the RVisio SoC (LEDs, buttons, display, UART, ADC)
port/, FreeRTOS-Kernel/   FreeRTOS V11.3.1 and the port for the RVisio SoC (same as demo/freertos-demo)
```
