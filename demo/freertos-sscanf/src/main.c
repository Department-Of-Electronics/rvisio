/*
 * main.c - FreeRTOS on the RVisio SoC: why sscanf() from a C library can be dangerous in an RTOS.
 *
 *   make DEMO=race   (default) two tasks with the same priority parse different lines with
 *                    unsafe_sscanf(), which keeps its parsing cursor in a static variable. The tick
 *                    switches tasks in the middle of a call, the other task overwrites the cursor, and the
 *                    first one continues parsing the OTHER task's string: a race condition. Every wrong
 *                    result is reported on the console.
 *   make DEMO=safe   the same tasks use the reentrant safe_sscanf_r() (state on the caller's stack):
 *                    no errors.
 *   make DEMO=stack  a task created with a small stack (192 words = 768 B) calls unsafe_sscanf(), which
 *                    puts a 512 B work buffer on the stack: the stack overflows into the memory below it
 *                    and the kernel stops the program (configCHECK_FOR_STACK_OVERFLOW = 2).
 *   make DEMO=crash  the same with an even smaller stack (150 words = 600 B): the overflow jumps over the
 *                    guard pattern at the bottom of the stack and overwrites the saved context of task
 *                    "victim". The kernel check does not see it; when "victim" is resumed, the program
 *                    jumps to a garbage address and crashes.
 *
 * In the simulator: SoC -> load build/app.elf -> "Run to end"; output in the Console panel. With the
 * "Advanced" panel layout, the "Stack analyzer" and "Stacks in memory" panels show the task switches in
 * the middle of parsing (race) and the stack overflow (stack).
 */
#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "soc.h"
#include "port_soc.h"
#include "mini_scanf.h"

#define DEMO_RACE   1
#define DEMO_SAFE   2
#define DEMO_STACK  3
#define DEMO_CRASH  4
#ifndef DEMO
#define DEMO DEMO_STACK
#endif

#if DEMO == DEMO_STACK
#define SSCANF safe_sscanf_r
#else
#define SSCANF unsafe_sscanf
#endif

#define ROUNDS 12                   /* lines each task parses */
#define SMALL_TASKS (DEMO == DEMO_STACK || DEMO == DEMO_CRASH)
#if DEMO == DEMO_CRASH
#define SMALL_STACK_WORDS 150       /* stack of task "small": 600 B */
#else
#define SMALL_STACK_WORDS 192       /* stack of task "small": 768 B */
#endif

static SemaphoreHandle_t xConsole;  /* console output of several tasks is protected by a mutex */
static volatile uint32_t ulErrors, ulDone;

/* ------------------------------------------------------------------ console (UART0) */

static void console_putc(char c)
{
    while (UART_STATUS & 4) { }
    UART_TXDATA = (uint8_t)c;
}

static void console_puts(const char *s)
{
    while (*s) console_putc(*s++);
}

static void console_putu(uint32_t v)
{
    char buf[11];
    int n = 0;
    do { buf[n++] = (char)('0' + v % 10); v /= 10; } while (v);
    while (n) console_putc(buf[--n]);
}

static void finish(void)
{
    console_puts("\n--- done: ");
    console_putu(ulErrors);
    console_puts(ulErrors ? " WRONG results (race condition in unsafe_sscanf)\n" : " wrong results\n");
    while (UART_STATUS & 1) { }
    taskDISABLE_INTERRUPTS();
    __asm volatile ("ebreak");
}

/* ------------------------------------------------------------------ race / safe: two parsers */
#if !SMALL_TASKS

typedef struct {
    char tag;               /* expected first field */
    const char *line;       /* text this task parses */
    uint32_t sum;           /* expected sum of the six numbers */
} ParserJob;

static const ParserJob xJobs[2] = {
    { 'A', "A 11 22 33 44 55 66", 231 },
    { 'B', "B 1000 2000 3000 4000 5000 6000", 21000 },
};

static void vParserTask(void *pv)
{
    const ParserJob *job = pv;
    for (int round = 1; round <= ROUNDS; round++) {
        char tag = '?';
        uint32_t v[6] = { 0 };
        int n = SSCANF(job->line, "%c %d %d %d %d %d %d", &tag, &v[0], &v[1], &v[2], &v[3], &v[4], &v[5]);
        uint32_t sum = v[0] + v[1] + v[2] + v[3] + v[4] + v[5];
        int ok = n == 7 && tag == job->tag && sum == job->sum;
        xSemaphoreTake(xConsole, portMAX_DELAY);
        console_putu(xTaskGetTickCount());
        console_puts(" ms  task "); console_putc(job->tag);
        console_puts(" round "); console_putu(round);
        console_puts(": fields "); console_putu((uint32_t)n);
        console_puts(", tag '"); console_putc(tag ? tag : '?');
        console_puts("', sum "); console_putu(sum);
        if (ok) console_puts("  ok\n");
        else {
            ulErrors++;
            console_puts("  WRONG (expected 7 fields, '"); console_putc(job->tag);
            console_puts("', "); console_putu(job->sum); console_puts(")\n");
        }
        xSemaphoreGive(xConsole);
    }
    taskENTER_CRITICAL();
    if (++ulDone == 2) finish();
    taskEXIT_CRITICAL();
    vTaskDelete(NULL);
}

#endif

/* ------------------------------------------------------------------ stack: small stack + sscanf */
#if SMALL_TASKS
/*
 * Heap layout (heap_4, the stack is allocated before the TCB, lower addresses first):
 *     [victim stack][victim TCB][small stack][small TCB]
 * The stack of "small" grows DOWN - the first thing below its bottom is the TCB of "victim".
 */
static void vVictimTask(void *pv)
{
    (void)pv;
    for (uint32_t beat = 1; ; beat++) {
        xSemaphoreTake(xConsole, portMAX_DELAY);
        console_putu(xTaskGetTickCount()); console_puts(" ms  victim: heartbeat "); console_putu(beat); console_putc('\n');
        xSemaphoreGive(xConsole);
        vTaskDelay(pdMS_TO_TICKS(3));
    }
}

static void vSmallStackTask(void *pv)
{
    (void)pv;
    xSemaphoreTake(xConsole, portMAX_DELAY);
    console_puts("task 'small': stack "); console_putu(SMALL_STACK_WORDS * 4); console_puts(" B, free before sscanf: ");
    console_putu(uxTaskGetStackHighWaterMark(NULL) * 4);
    console_puts(" B; unsafe_sscanf() needs a 512 B work buffer per number ...\n");
    xSemaphoreGive(xConsole);
    for (uint32_t round = 1; round <= 5; round++) {
        uint32_t a = 0, b = 0;
        int n = unsafe_sscanf("12 34", "%d %d", &a, &b);
        /* without the kernel check nothing stops the task here - it continues with damaged memory */
        xSemaphoreTake(xConsole, portMAX_DELAY);
        console_putu(xTaskGetTickCount()); console_puts(" ms  small: round "); console_putu(round);
        console_puts(", parsed "); console_putu((uint32_t)n); console_puts(" fields: ");
        console_putu(a); console_puts(" "); console_putu(b);
        console_puts(", free stack "); console_putu(uxTaskGetStackHighWaterMark(NULL) * 4); console_puts(" B\n");
        xSemaphoreGive(xConsole);
        vTaskDelay(pdMS_TO_TICKS(2));   /* task switch -> with OVERFLOW_CHECK=2 the kernel checks the stack here */
    }
    finish();
}
#endif

/* ------------------------------------------------------------------ main */

int main(void)
{
    console_puts("FreeRTOS " tskKERNEL_VERSION_NUMBER " - sscanf in an RTOS: ");
    console_puts(DEMO == DEMO_RACE ? "RACE (unsafe_sscanf)\n" : DEMO == DEMO_SAFE ? "SAFE (safe_sscanf_r)\n" 
                 : DEMO == DEMO_STACK ? "STACK (unsafe_sscanf on a small stack)\n" : "CRASH (unsafe_sscanf on an even smaller stack)\n");
    xConsole = xSemaphoreCreateMutex();
    configASSERT(xConsole);

#if SMALL_TASKS
    xTaskCreate(vVictimTask, "victim", 256, NULL, 1, NULL);   /* first: its TCB lies right below the stack of "small" */
    xTaskCreate(vSmallStackTask, "small", SMALL_STACK_WORDS, NULL, 2, NULL);
#else
    /* same priority: the scheduler time-slices them on every tick (configUSE_TIME_SLICING = 1) */
    xTaskCreate(vParserTask, "parser A", 512, (void *)&xJobs[0], 1, NULL);
    xTaskCreate(vParserTask, "parser B", 512, (void *)&xJobs[1], 1, NULL);
#endif

    vTaskStartScheduler();
    for (;;) { }
}

/* ------------------------------------------------------------------ hooks and errors */

static void halt(const char *what, const char *info, unsigned long line)
{
    taskDISABLE_INTERRUPTS();
    console_puts("\n*** "); console_puts(what); console_puts(info);
    if (line) { console_putc(':'); console_putu(line); }
    console_putc('\n');
    while (UART_STATUS & 1) { }
    __asm volatile ("ebreak");
    for (;;) { }
}

void vAssertCalled(const char *file, unsigned long line) { halt("configASSERT: ", file, line); }
void vApplicationMallocFailedHook(void) { halt("out of heap memory", "", 0); }
void vApplicationStackOverflowHook(TaskHandle_t task, char *name) { (void)task; halt("STACK OVERFLOW in task: ", name, 0); }
