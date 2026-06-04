#ifndef UTILS_H
#define UTILS_H

#include <string.h>

#define EVAL_ERR_NE_GOTO(EC, ERR, LABEL)                                   \
    if ((EC) != (ERR)) goto LABEL;

#define EVAL_ERR_EQ_GOTO(EC, ERR, LABEL)                                   \
    if ((EC) == (ERR)) goto LABEL;

#define SIZEOF_ARRAY(a)     (sizeof(a) / sizeof((a)[0]))
#ifndef MAX
#define MAX(a, b)           (((a) > (b)) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a, b)           (((a) < (b)) ? (a) : (b))
#endif
#define BITMASK_1           (0x01)
#define BITMASK_2           (0x03)
#define BITMASK_3           (0x07)
#define BITMASK_4           (0x0F)
#define U16TOU8(a)          ((a) & 0x00FF)
#define GETU16(a)           (((uint16_t)(a)[0] << 8) | (uint16_t)(a)[1])
#define GETU32(a)           (((uint32_t)(a)[0] << 24) | ((uint32_t)(a)[1] << 16) | ((uint32_t)(a)[2] << 8) | ((uint32_t)(a)[3]))

#define REVERSE_BYTES(pData, nDataSize) \
  {unsigned char swap, *lo = ((unsigned char *)(pData)), *hi = ((unsigned char *)(pData)) + (nDataSize) - 1; \
  while (lo < hi) { swap = *lo; *lo++ = *hi; *hi-- = swap; }}

#ifdef __CSMC__

#define ST_MEMMOVE(s1,s2,n)                                                 memmove(s1,s2,n)
static inline void * ST_MEMCPY(void *s1, const void *s2, uint32_t n)      { return memcpy(s1,s2,(uint16_t)n); }
#define ST_MEMSET(s1,c,n)                                                   memset(s1,(char)(c),n)
static inline int32_t ST_BYTECMP(void *s1, const void *s2, uint32_t n)    { return (int32_t)memcmp(s1,s2,(uint16_t)n); }

#else

#define ST_MEMMOVE          memmove
#define ST_MEMCPY           memcpy
#define ST_MEMSET           memset
#define ST_BYTECMP          memcmp
#endif

#define NO_WARNING(v)      ((void) (v))

#ifndef NULL
  #define NULL (void*)0
#endif

#endif

