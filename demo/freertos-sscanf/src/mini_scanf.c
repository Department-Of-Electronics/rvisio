/*
 * mini_scanf.c - a small sscanf() look-alike with the same two problems as many real C-library scanf
 * implementations used from an RTOS without care:
 *
 *  1. NOT THREAD-SAFE: unsafe_sscanf() keeps its parsing state (input cursor, conversion count, last
 *     error) in STATIC variables shared by every caller - like strtok(), or like a C library whose
 *     reentrancy structure is global. If a task is preempted in the middle of a call and another task
 *     calls unsafe_sscanf(), both continue with the other one's cursor: fields are lost, swapped or read
 *     from the wrong string.
 *
 *  2. STACK HUNGRY: every call allocates a large work buffer on the caller's stack (real implementations
 *     need buffers for number conversion, character classes, wide characters ...). A task created with a
 *     small stack overflows as soon as it calls it.
 *
 * safe_sscanf_r() is the reentrant fix: all state is in a context owned by the caller and the work
 * buffer is small.
 *
 * Supported conversions: %d %u %x %s %c and literal characters; whitespace in the format skips
 * whitespace in the input.
 */
#include <stdarg.h>
#include <stdint.h>
#include "mini_scanf.h"

/* ------------------------------------------------------------------ shared (unsafe) state */

static const char *s_pos;       /* where parsing continues - shared by ALL callers */
static int s_count;             /* number of converted fields so far */

/* simulates the cost of a real conversion (locale handling, overflow checks ...): parsing takes long
 * enough that the RTOS tick can preempt a task in the middle of a call */
static void slow_step(void)
{
    for (volatile int i = 0; i < SCANF_DELAY; i++) { }
}

static int is_space(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }

static int digit_value(char c, int base)
{
    int v = (c >= '0' && c <= '9') ? c - '0'
          : (c >= 'a' && c <= 'f') ? c - 'a' + 10
          : (c >= 'A' && c <= 'F') ? c - 'A' + 10 : 99;
    return v < base ? v : -1;
}

/* one numeric conversion from the SHARED cursor */
static int read_number(int base, int is_signed, uint32_t *out)
{
    char work[SCANF_WORK_BYTES];    /* large stack buffer, like a real scanf conversion buffer */
    int n = 0, neg = 0;
    while (is_space(*s_pos)) s_pos++;
    if (is_signed && (*s_pos == '-' || *s_pos == '+')) { neg = *s_pos == '-'; s_pos++; }
    while (digit_value(*s_pos, base) >= 0 && n < (int)sizeof work - 1) {
        work[n++] = *s_pos++;
        slow_step();                /* the tick may switch tasks here - s_pos is shared! */
    }
    if (n == 0) return 0;
    uint32_t v = 0;
    for (int i = 0; i < n; i++) v = v * (uint32_t)base + (uint32_t)digit_value(work[i], base);
    *out = neg ? (uint32_t)(-(int32_t)v) : v;
    return 1;
}

int unsafe_sscanf(const char *str, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    s_pos = str;
    s_count = 0;
    for (; *fmt; fmt++) {
        if (is_space(*fmt)) { while (is_space(*s_pos)) s_pos++; continue; }
        if (*fmt != '%') { if (*s_pos != *fmt) break; s_pos++; continue; }
        fmt++;
        uint32_t v;
        if (*fmt == 'd' || *fmt == 'u' || *fmt == 'x') {
            if (!read_number(*fmt == 'x' ? 16 : 10, *fmt == 'd', &v)) break;
            *va_arg(ap, uint32_t *) = v;
        } else if (*fmt == 's') {
            char *dst = va_arg(ap, char *);
            while (is_space(*s_pos)) s_pos++;
            while (*s_pos && !is_space(*s_pos)) { *dst++ = *s_pos++; slow_step(); }
            *dst = 0;
        } else if (*fmt == 'c') {
            *va_arg(ap, char *) = *s_pos ? *s_pos++ : 0;
        } else {
            break;
        }
        s_count++;
    }
    va_end(ap);
    return s_count;
}

/* ------------------------------------------------------------------ reentrant version */

static int r_number(const char **pos, int base, int is_signed, uint32_t *out)
{
    char work[16];                  /* small: enough for a 32-bit number */
    int n = 0, neg = 0;
    while (is_space(**pos)) (*pos)++;
    if (is_signed && (**pos == '-' || **pos == '+')) { neg = **pos == '-'; (*pos)++; }
    while (digit_value(**pos, base) >= 0 && n < (int)sizeof work - 1) {
        work[n++] = *(*pos)++;
        slow_step();                /* same speed - but the cursor belongs to this call only */
    }
    if (n == 0) return 0;
    uint32_t v = 0;
    for (int i = 0; i < n; i++) v = v * (uint32_t)base + (uint32_t)digit_value(work[i], base);
    *out = neg ? (uint32_t)(-(int32_t)v) : v;
    return 1;
}

int safe_sscanf_r(const char *str, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    const char *pos = str;          /* state lives on the caller's own stack */
    int count = 0;
    for (; *fmt; fmt++) {
        if (is_space(*fmt)) { while (is_space(*pos)) pos++; continue; }
        if (*fmt != '%') { if (*pos != *fmt) break; pos++; continue; }
        fmt++;
        uint32_t v;
        if (*fmt == 'd' || *fmt == 'u' || *fmt == 'x') {
            if (!r_number(&pos, *fmt == 'x' ? 16 : 10, *fmt == 'd', &v)) break;
            *va_arg(ap, uint32_t *) = v;
        } else if (*fmt == 's') {
            char *dst = va_arg(ap, char *);
            while (is_space(*pos)) pos++;
            while (*pos && !is_space(*pos)) { *dst++ = *pos++; slow_step(); }
            *dst = 0;
        } else if (*fmt == 'c') {
            *va_arg(ap, char *) = *pos ? *pos++ : 0;
        } else {
            break;
        }
        count++;
    }
    va_end(ap);
    return count;
}
