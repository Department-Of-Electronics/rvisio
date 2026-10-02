/* mini_scanf.h - sscanf() look-alikes: one NOT thread-safe and stack hungry, one reentrant. */
#ifndef MINI_SCANF_H
#define MINI_SCANF_H

/* work buffer that unsafe_sscanf() puts on the caller's stack for every numeric conversion */
#ifndef SCANF_WORK_BYTES
#define SCANF_WORK_BYTES 512
#endif

/* busy-wait per parsed character (cost of a "real" conversion) - widens the race window */
#ifndef SCANF_DELAY
#define SCANF_DELAY 40
#endif

/* NOT thread-safe: parsing state is shared by all callers (static variables). */
int unsafe_sscanf(const char *str, const char *fmt, ...);

/* Reentrant: all state belongs to the call; small stack usage. */
int safe_sscanf_r(const char *str, const char *fmt, ...);

#endif
