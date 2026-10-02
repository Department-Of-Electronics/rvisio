# FreeRTOS – sscanf() is not thread-safe (race condition and stack overflow)

> **Run it in the browser:** [rvisio.com](https://rvisio.com) → Open simulator → **System on chip (SoC)** →
> **📦 FreeRTOS demos** → *sscanf: a race between tasks (and the other three sscanf demos)* → **▶ Run** (no toolchain needed).


Many C-library functions were written before RTOSes and threads: `strtok()`, `rand()`, `asctime()`,
and – in C libraries built without reentrancy support – `sscanf()`/`printf()` keep state in **static
(shared) variables** and use **large buffers on the caller's stack**. In an RTOS this causes two classic
bugs, both shown here on the RVisio SoC.

The toolchain used for the simulator has no C library, so `src/mini_scanf.c` provides a small
`sscanf()` look-alike with exactly these properties:

| function | state | stack per number | thread-safe |
|---|---|---|---|
| `unsafe_sscanf()` | static `s_pos`, `s_count` shared by all callers | 512 B work buffer | **no** |
| `safe_sscanf_r()` | local variables of the call | 16 B | yes |

Each parsed character costs some time (`SCANF_DELAY`), like a real conversion, so a call lasts several
RTOS ticks and the scheduler can switch tasks in the middle of it.

## Four programs (`make DEMO=…`)

```bash
make DEMO=race                     # default: race condition
make DEMO=safe                     # the fix: reentrant parser
make DEMO=stack                    # stack overflow, caught by the kernel
make DEMO=stack OVERFLOW_CHECK=0   # the same overflow without the check: nobody notices
make DEMO=crash                    # bigger overflow: skips the kernel check and crashes the program
```

### race – two tasks, one shared cursor

Tasks `parser A` and `parser B` have the same priority, so the tick time-slices them. Each parses its
own line 12 times and checks the result:

```
A 11 22 33 44 55 66                 -> tag 'A', sum 231
B 1000 2000 3000 4000 5000 6000     -> tag 'B', sum 21000
```

When A is preempted inside `unsafe_sscanf()`, B starts its own call and sets the shared cursor `s_pos`
to B's string. A then continues **from B's string** (and B from wherever A left it): fields are lost or
taken from the other line, and the console reports `WRONG …` results. The bug depends on timing – it
appears in some rounds and not in others, and changes with the core or cache configuration.

### safe – reentrant parser

The same two tasks with `safe_sscanf_r()`: the cursor is a local variable of each call (on that task's
own stack), so preemption does not matter – `0 wrong results`. Other fixes: protect the call with a mutex
(`xSemaphoreTake/Give` around every `sscanf`), use only one task for parsing, or use a C library built
with reentrancy support (newlib with `configUSE_NEWLIB_REENTRANT = 1`, picolibc with TLS).

### stack – sscanf on a small stack

Two tasks are created: `victim` first (priority 1, prints a heartbeat every 3 ms), then `small`
(priority 2). heap_4 allocates a task's stack before its TCB, so the memory is
`[victim stack][victim TCB][small stack][small TCB]` – right below the bottom of the `small` stack is
the TCB of `victim`.

Task `small` has 192 words (768 B) – enough for its own code, but `unsafe_sscanf()` puts a 512 B buffer
on the stack for every number, plus its call frames and the 124 B context frame saved by every tick
interrupt. The stack grows below its bottom. At the next task switch the kernel sees it
(`configCHECK_FOR_STACK_OVERFLOW = 2`):

```
5 ms  small: round 1, parsed 2 fields: 12 34, free stack 0 B

*** STACK OVERFLOW in task: small
```

With `OVERFLOW_CHECK=0` the program runs to the end and prints correct results – the overflow only hit
the tail of the `victim` TCB and a heap block header. Memory is damaged, but nothing shows it: the most
dangerous kind of bug, it will bite later, after an unrelated change.

### crash – the overflow jumps over the guard

The same program with 150 words (600 B). Now the overflow is deep enough to pass the whole `victim` TCB
and reach the top of the `victim` stack, where its saved context (registers, `mepc`) is stored. The
kernel check does NOT catch it, although it is enabled:

* the 512 B buffer is only *reserved* – `sp` moves 512 B down, but the buffer is written only where the
  digits go. The 0xA5 guard pattern at the bottom of the stack is jumped over and stays intact;
* the check runs at a task switch, when `sp` is already back inside the stack;
* `uxTaskGetStackHighWaterMark()` measures the intact pattern too, so it reports `free stack 372 B`
  even right after the overflow.

When `small` blocks and the kernel resumes `victim`, it loads the destroyed context and `mret` jumps
to a garbage address. The console just stops – no message, no ebreak, the processor executes
whatever it finds. Only a hardware guard (MPU/PMP region below the stack) or a big enough stack
prevents this.

## What to look at in the simulator

SoC → "📂 ELF" → `build/app.elf` → "Run to end", panel layout **Advanced**:

* **Console** – results of every round, `WRONG` lines in the race program, the overflow message.
* **Stack analyzer** – both parsers alternate during parsing (the colour of the line changes in the middle
  of a call); in the stack and crash programs the reserve of task `small` drops below 0 – the analyzer
  sees every `sp`, so it catches the overflow the kernel misses in the crash program.
* **Stacks in memory** – the line of `small` goes below the bottom of its band (red), into the band of `victim`.
* From VS Code (GDB server, F5): a breakpoint in `read_number()` shows `s_pos` pointing into the other
  task's string after a task switch.

Files: `src/main.c` (the three programs), `src/mini_scanf.c/.h` (the parsers); kernel, port, linker
script and startup code are the same as in `../freertos-template`.
