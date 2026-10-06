# FreeRTOS – stack overflow

> **Run it in the browser:** [rvisio.com](https://rvisio.com) → Open simulator → **System on chip (SoC)** →
> **📦 FreeRTOS demos** → *Stack overflow (recursion)* → **⬇ Load program** → **▶ Run** (no toolchain needed).


A demonstration of what happens when a task uses more stack than it was given, and how FreeRTOS
and the simulator's stack analyzer detect it. The kernel and the port are the same as in `../freertos-template`.

## Program (`src/main.c`)

* **"counter"** (priority 1, stack 192 words = 768 B) – created FIRST. For a downward-growing stack FreeRTOS
  allocates the stack first and then the TCB, so in the heap (lower addresses on top → higher below) they are:
  `[counter stack][counter TCB][deep stack][deep TCB]`. The stack of task "deep" is right ABOVE the TCB
  of task "counter". "counter" counts and checks 8 "canaries" at the top of its stack.
* **"deep"** (priority 2, stack 768 B) – every 10 ms it recurses one level deeper (each level
  ~64 B: a 12-word array, return address, saved registers) and prints `sp` and the minimum free stack
  (`uxTaskGetStackHighWaterMark`). After about ten steps the stack passes its bottom.

## Two outcomes

```bash
make                    # check ENABLED (configCHECK_FOR_STACK_OVERFLOW = 2) – default
make OVERFLOW_CHECK=0   # check DISABLED
```

**With the check** (`= 2`): on every task switch the kernel checks whether `sp` is below the bottom of the stack and whether
the last 16 B of the stack (filled with the 0xA5 pattern when the task was created) are intact. After the step in
which the recursion passes the bottom, it calls `vApplicationStackOverflowHook`:

```
...
81 ms  depth 9: sp 0x00010be0, minimum free stack 80 B (counter: 836)
91 ms  depth 10: sp 0x00010be0, minimum free stack 12 B (counter: 951)

*** STACK OVERFLOW in task: deep
```

**Without the check** (`= 0`) – what really happens in a program without protection:
1. the recursion writes below the bottom of the "deep" stack – over the heap block header and the **TCB of task "counter"**;
2. a timer interrupt in the middle of the recursion saves the context (124 B) even deeper into foreign memory;
3. the return from the interrupt loads the context from overwritten memory (`mepc = 0`) and `mret` jumps to address 0 –
   the program restarts "by itself", while the timer keeps sending interrupts to an uninitialized system.

The output stops after `depth 10` without any message – the error is not visible where it occurred, which is
typical of a stack overflow. (The canaries of task "counter" are below its TCB; here the system crashes
before the overflow reaches them – with a shallower overflow "counter" would report them.)

## What to watch in the simulator

SoC → "📂 ELF" → `build/app.elf` → "⏭ To end"; panel layout **Advanced**:

* **Console**: the recursion steps and the overflow message.
* **Stack analyzer**: the "deep" stack grows step by step; in the stack table the margin drops to 0 and the row turns
  red ("Overflow: task "deep" – sp … below the bottom of the stack …"). Zoom (drag with the mouse) around the end.
* **Stacks in memory**: the "deep" bar fills step by step, and then a red part crosses below
  its bottom – into the memory below (the TCB and the stack of task "counter").
* From VS Code (🐞 GDB server, F5): a breakpoint in `prvRecurse` or `vApplicationStackOverflowHook`;
  the analyzer also recognizes the tasks when the program is loaded by GDB.

## How to prevent overflow

* a large enough stack (`xTaskCreate(..., number_of_words, ...)`) – measure it with `uxTaskGetStackHighWaterMark` or
  the stack analyzer, then add a margin;
* no recursion and no large local arrays in tasks (use static ones or the heap);
* `configCHECK_FOR_STACK_OVERFLOW = 2` during development; on cores with an MPU/PMP – a guard page below the
  stack (an access raises an exception immediately, not only at the next task switch).
