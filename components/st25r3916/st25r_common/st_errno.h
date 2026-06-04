#ifndef ST_ERRNO_H
#define ST_ERRNO_H

typedef uint16_t      ReturnCodeA;

#define ERR_NONE                           ((ReturnCodeA)0U)
#define ERR_NOMEM                          ((ReturnCodeA)1U)
#define ERR_BUSY                           ((ReturnCodeA)2U)
#define ERR_IO                             ((ReturnCodeA)3U)
#define ERR_TIMEOUT                        ((ReturnCodeA)4U)
#define ERR_REQUEST                        ((ReturnCodeA)5U)
#define ERR_NOMSG                          ((ReturnCodeA)6U)
#define ERR_PARAM                          ((ReturnCodeA)7U)
#define ERR_SYSTEM                         ((ReturnCodeA)8U)
#define ERR_FRAMING                        ((ReturnCodeA)9U)
#define ERR_OVERRUN                        ((ReturnCodeA)10U)
#define ERR_PROTO                          ((ReturnCodeA)11U)
#define ERR_INTERNAL                       ((ReturnCodeA)12U)
#define ERR_AGAIN                          ((ReturnCodeA)13U)
#define ERR_MEM_CORRUPT                    ((ReturnCodeA)14U)
#define ERR_NOT_IMPLEMENTED                ((ReturnCodeA)15U)
#define ERR_PC_CORRUPT                     ((ReturnCodeA)16U)
#define ERR_SEND                           ((ReturnCodeA)17U)
#define ERR_IGNORE                         ((ReturnCodeA)18U)
#define ERR_SEMANTIC                       ((ReturnCodeA)19U)
#define ERR_SYNTAX                         ((ReturnCodeA)20U)
#define ERR_CRC                            ((ReturnCodeA)21U)
#define ERR_NOTFOUND                       ((ReturnCodeA)22U)
#define ERR_NOTUNIQUE                      ((ReturnCodeA)23U)
#define ERR_NOTSUPP                        ((ReturnCodeA)24U)
#define ERR_WRITE                          ((ReturnCodeA)25U)
#define ERR_FIFO                           ((ReturnCodeA)26U)
#define ERR_PAR                            ((ReturnCodeA)27U)
#define ERR_DONE                           ((ReturnCodeA)28U)
#define ERR_RF_COLLISION                   ((ReturnCodeA)29U)
#define ERR_HW_OVERRUN                     ((ReturnCodeA)30U)
#define ERR_RELEASE_REQ                    ((ReturnCodeA)31U)
#define ERR_SLEEP_REQ                      ((ReturnCodeA)32U)
#define ERR_WRONG_STATE                    ((ReturnCodeA)33U)
#define ERR_MAX_RERUNS                     ((ReturnCodeA)34U)
#define ERR_DISABLED                       ((ReturnCodeA)35U)
#define ERR_HW_MISMATCH                    ((ReturnCodeA)36U)
#define ERR_LINK_LOSS                      ((ReturnCodeA)37U)
#define ERR_INVALID_HANDLE                 ((ReturnCodeA)38U)

#define ERR_INCOMPLETE_BYTE                ((ReturnCodeA)40U)
#define ERR_INCOMPLETE_BYTE_01             ((ReturnCodeA)41U)
#define ERR_INCOMPLETE_BYTE_02             ((ReturnCodeA)42U)
#define ERR_INCOMPLETE_BYTE_03             ((ReturnCodeA)43U)
#define ERR_INCOMPLETE_BYTE_04             ((ReturnCodeA)44U)
#define ERR_INCOMPLETE_BYTE_05             ((ReturnCodeA)45U)
#define ERR_INCOMPLETE_BYTE_06             ((ReturnCodeA)46U)
#define ERR_INCOMPLETE_BYTE_07             ((ReturnCodeA)47U)

#define ERR_GENERIC_GRP                     (0x0000)
#define ERR_WARN_GRP                        (0x0100)
#define ERR_PROCESS_GRP                     (0x0200)
#define ERR_SIO_GRP                         (0x0800)
#define ERR_RINGBUF_GRP                     (0x0900)
#define ERR_MQ_GRP                          (0x0A00)
#define ERR_TIMER_GRP                       (0x0B00)
#define ERR_RFAL_GRP                        (0x0C00)
#define ERR_UART_GRP                        (0x0D00)
#define ERR_SPI_GRP                         (0x0E00)
#define ERR_I2C_GRP                         (0x0F00)

#define ERR_INSERT_SIO_GRP(x)               (ERR_SIO_GRP     | (x))
#define ERR_INSERT_RINGBUF_GRP(x)           (ERR_RINGBUF_GRP | (x))
#define ERR_INSERT_RFAL_GRP(x)              (ERR_RFAL_GRP    | (x))
#define ERR_INSERT_SPI_GRP(x)               (ERR_SPI_GRP     | (x))
#define ERR_INSERT_I2C_GRP(x)               (ERR_I2C_GRP     | (x))
#define ERR_INSERT_UART_GRP(x)              (ERR_UART_GRP    | (x))
#define ERR_INSERT_TIMER_GRP(x)             (ERR_TIMER_GRP   | (x))
#define ERR_INSERT_MQ_GRP(x)                (ERR_MQ_GRP      | (x))
#define ERR_INSERT_PROCESS_GRP(x)           (ERR_PROCESS_GRP | (x))
#define ERR_INSERT_WARN_GRP(x)              (ERR_WARN_GRP    | (x))
#define ERR_INSERT_GENERIC_GRP(x)           (ERR_GENERIC_GRP | (x))

#define ERR_NO_MASK(x)                      ((uint16_t)(x) & 0x00FFU)

#define EXIT_ON_ERR(r, f) \
    (r) = (f);            \
    if (ERR_NONE != (r))  \
    {                     \
        return (r);       \
    }

#define EXIT_ON_BUSY(r, f) \
    (r) = (f);            \
    if (ERR_BUSY == (r))  \
    {                     \
        return (r);       \
    }
#endif

