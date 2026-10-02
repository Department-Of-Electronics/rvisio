/* string.h – minimal version for the FreeRTOS kernel; the implementation is in src/libc_min.c. */
#ifndef MIN_STRING_H
#define MIN_STRING_H
#include <stddef.h>
void * memset( void * d, int c, size_t n );
void * memcpy( void * d, const void * s, size_t n );
int memcmp( const void * a, const void * b, size_t n );
size_t strlen( const char * s );
#endif
