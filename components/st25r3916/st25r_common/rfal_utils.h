#ifndef RFAL_UTILS_H
#define RFAL_UTILS_H

#include <stdint.h>
#include <string.h>

typedef uint16_t      ReturnCode;

#define RFAL_ERR_NONE                           ((ReturnCode)0U)
#define RFAL_ERR_NOMEM                          ((ReturnCode)1U)
#define RFAL_ERR_BUSY                           ((ReturnCode)2U)
#define RFAL_ERR_IO                             ((ReturnCode)3U)
#define RFAL_ERR_TIMEOUT                        ((ReturnCode)4U)
#define RFAL_ERR_REQUEST                        ((ReturnCode)5U)
#define RFAL_ERR_NOMSG                          ((ReturnCode)6U)
#define RFAL_ERR_PARAM                          ((ReturnCode)7U)
#define RFAL_ERR_SYSTEM                         ((ReturnCode)8U)
#define RFAL_ERR_FRAMING                        ((ReturnCode)9U)
#define RFAL_ERR_OVERRUN                        ((ReturnCode)10U)
#define RFAL_ERR_PROTO                          ((ReturnCode)11U)
#define RFAL_ERR_INTERNAL                       ((ReturnCode)12U)
#define RFAL_ERR_AGAIN                          ((ReturnCode)13U)
#define RFAL_ERR_MEM_CORRUPT                    ((ReturnCode)14U)
#define RFAL_ERR_NOT_IMPLEMENTED                ((ReturnCode)15U)
#define RFAL_ERR_PC_CORRUPT                     ((ReturnCode)16U)
#define RFAL_ERR_SEND                           ((ReturnCode)17U)
#define RFAL_ERR_IGNORE                         ((ReturnCode)18U)
#define RFAL_ERR_SEMANTIC                       ((ReturnCode)19U)
#define RFAL_ERR_SYNTAX                         ((ReturnCode)20U)
#define RFAL_ERR_CRC                            ((ReturnCode)21U)
#define RFAL_ERR_NOTFOUND                       ((ReturnCode)22U)
#define RFAL_ERR_NOTUNIQUE                      ((ReturnCode)23U)
#define RFAL_ERR_NOTSUPP                        ((ReturnCode)24U)
#define RFAL_ERR_WRITE                          ((ReturnCode)25U)
#define RFAL_ERR_FIFO                           ((ReturnCode)26U)
#define RFAL_ERR_PAR                            ((ReturnCode)27U)
#define RFAL_ERR_DONE                           ((ReturnCode)28U)
#define RFAL_ERR_RF_COLLISION                   ((ReturnCode)29U)
#define RFAL_ERR_HW_OVERRUN                     ((ReturnCode)30U)
#define RFAL_ERR_RELEASE_REQ                    ((ReturnCode)31U)
#define RFAL_ERR_SLEEP_REQ                      ((ReturnCode)32U)
#define RFAL_ERR_WRONG_STATE                    ((ReturnCode)33U)
#define RFAL_ERR_MAX_RERUNS                     ((ReturnCode)34U)
#define RFAL_ERR_DISABLED                       ((ReturnCode)35U)
#define RFAL_ERR_HW_MISMATCH                    ((ReturnCode)36U)
#define RFAL_ERR_LINK_LOSS                      ((ReturnCode)37U)
#define RFAL_ERR_INVALID_HANDLE                 ((ReturnCode)38U)

#define RFAL_ERR_INCOMPLETE_BYTE                ((ReturnCode)40U)
#define RFAL_ERR_INCOMPLETE_BYTE_01             ((ReturnCode)41U)
#define RFAL_ERR_INCOMPLETE_BYTE_02             ((ReturnCode)42U)
#define RFAL_ERR_INCOMPLETE_BYTE_03             ((ReturnCode)43U)
#define RFAL_ERR_INCOMPLETE_BYTE_04             ((ReturnCode)44U)
#define RFAL_ERR_INCOMPLETE_BYTE_05             ((ReturnCode)45U)
#define RFAL_ERR_INCOMPLETE_BYTE_06             ((ReturnCode)46U)
#define RFAL_ERR_INCOMPLETE_BYTE_07             ((ReturnCode)47U)

#define RFAL_EXIT_ON_ERR(r, f) \
    (r) = (f);                 \
    if (RFAL_ERR_NONE != (r))  \
    {                          \
        return (r);            \
    }

#define RFAL_EXIT_ON_BUSY(r, f) \
    (r) = (f);                  \
    if (RFAL_ERR_BUSY == (r))   \
    {                           \
        return (r);             \
    }

#define RFAL_SIZEOF_ARRAY(a)     (sizeof(a) / sizeof((a)[0]))
#define RFAL_MAX(a, b)           (((a) > (b)) ? (a) : (b))
#define RFAL_MIN(a, b)           (((a) < (b)) ? (a) : (b))
#define RFAL_GETU16(a)           (((uint16_t)(a)[0] << 8) | (uint16_t)(a)[1])
#define RFAL_GETU32(a)           (((uint32_t)(a)[0] << 24) | ((uint32_t)(a)[1] << 16) | ((uint32_t)(a)[2] << 8) | ((uint32_t)(a)[3]))

#ifdef __CSMC__

#define RFAL_MEMMOVE(s1,s2,n)                                                 memmove(s1,s2,n)
static inline void * RFAL_MEMCPY(void *s1, const void *s2, uint32_t n)      { return memcpy(s1,s2,(uint16_t)n); }
#define RFAL_MEMSET(s1,c,n)                                                   memset(s1,(char)(c),n)
static inline int32_t RFAL_BYTECMP(void *s1, const void *s2, uint32_t n)    { return (int32_t)memcmp(s1,s2,(uint16_t)n); }

#else

#define RFAL_MEMMOVE          memmove
#define RFAL_MEMCPY           memcpy
#define RFAL_MEMSET           memset
#define RFAL_BYTECMP          memcmp
#endif

#define RFAL_NO_WARNING(v)      ((void) (v))

#ifndef NULL
  #define NULL (void*)0
#endif

#endif

