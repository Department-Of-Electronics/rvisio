# Interrupts without an operating system (timer + UART)

> **Run it in the browser:** [rvisio.com](https://rvisio.com) → Open simulator → **System on chip (SoC)** →
> **📦 FreeRTOS demos** → *Interrupts without an operating system* → **⬇ Load program** → **▶ Run**.

A bare-metal C program for the RVisio SoC: the main program "does work" (counts in a loop) and two
interrupts interrupt it through the interrupt controller (INTC):

* **TIMER0** every 2000 cycles: the handler increments `ticks` and shows it on the LEDs (GPIO);
* **UART0** receive: type a character in the **Console** panel and the handler echoes it in upper case.

After 20 timer interrupts the program prints the result and stops.

```
main.c      the program and the interrupt handler (trap_handler)
startup.S   reset: stack, mtvec = trap_handler, .data/.bss, main
link.ld     Flash 0x0 (code), SRAM 0x10000 (data, stack)
Makefile    make -> build/app.elf, build/app.lst
.vscode/    debugging from VS Code via the simulator's GDB server
```

## Build and run

```bash
make            # build/app.elf
```

RVisio → **System on chip (SoC)** → **📂 ELF** → `build/app.elf` → **⏭ To end**.

What to watch:

* **Peripheral events and interrupts**: every timer and UART interrupt, when it was requested and handled;
* **Processor**: `mepc` (where the main program was interrupted) and `mcause` (which interrupt) inside the handler;
* **Stack analyzer**: the handler saves registers on the stack and restores them before `mret`;
* **Sensors and actuators**: the LEDs count the timer interrupts.

## Debug from VS Code

Click **🐞 GDB server** in RVisio, open this folder in VS Code and press **F5**. Put a breakpoint in
`trap_handler` (for example on `ticks++`), then look at the Processor and Stack panels in the browser
while you step.

The addresses and interrupt numbers come from the default SoC configuration (RVisio: **Memory map** panel,
or SoC → **📄 Datasheet**).
