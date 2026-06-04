#ifndef RFAL_FEATURES_H
#define RFAL_FEATURES_H

#include "rfal_platform.h"

#define RFAL_SUPPORT_MODE_POLL_NFCA                true
#define RFAL_SUPPORT_MODE_POLL_NFCB                false
#define RFAL_SUPPORT_MODE_POLL_NFCF                false
#define RFAL_SUPPORT_MODE_POLL_NFCV                false
#define RFAL_SUPPORT_MODE_POLL_ACTIVE_P2P          false
#define RFAL_SUPPORT_MODE_LISTEN_NFCA              true
#define RFAL_SUPPORT_MODE_LISTEN_NFCB              false
#define RFAL_SUPPORT_MODE_LISTEN_NFCF              false
#define RFAL_SUPPORT_MODE_LISTEN_ACTIVE_P2P        false

#define RFAL_SUPPORT_CE                            ( RFAL_SUPPORT_MODE_LISTEN_NFCA || RFAL_SUPPORT_MODE_LISTEN_NFCB || RFAL_SUPPORT_MODE_LISTEN_NFCF )

#define RFAL_SUPPORT_RW                            ( RFAL_SUPPORT_MODE_POLL_NFCA || RFAL_SUPPORT_MODE_POLL_NFCB || RFAL_SUPPORT_MODE_POLL_NFCF || RFAL_SUPPORT_MODE_POLL_NFCV )

#define RFAL_SUPPORT_AP2P                          ( RFAL_SUPPORT_MODE_POLL_ACTIVE_P2P || RFAL_SUPPORT_MODE_LISTEN_ACTIVE_P2P )

#define RFAL_SUPPORT_BR_RW_106                      true
#define RFAL_SUPPORT_BR_RW_212                      true
#define RFAL_SUPPORT_BR_RW_424                      true
#define RFAL_SUPPORT_BR_RW_848                      true
#define RFAL_SUPPORT_BR_RW_1695                     false
#define RFAL_SUPPORT_BR_RW_3390                     false
#define RFAL_SUPPORT_BR_RW_6780                     false
#define RFAL_SUPPORT_BR_RW_13560                    false

#define RFAL_SUPPORT_BR_AP2P_106                    true
#define RFAL_SUPPORT_BR_AP2P_212                    true
#define RFAL_SUPPORT_BR_AP2P_424                    true
#define RFAL_SUPPORT_BR_AP2P_848                    false

#define RFAL_SUPPORT_BR_CE_A_106                    true
#define RFAL_SUPPORT_BR_CE_A_212                    false
#define RFAL_SUPPORT_BR_CE_A_424                    false
#define RFAL_SUPPORT_BR_CE_A_848                    false

#define RFAL_SUPPORT_BR_CE_B_106                    false
#define RFAL_SUPPORT_BR_CE_B_212                    false
#define RFAL_SUPPORT_BR_CE_B_424                    false
#define RFAL_SUPPORT_BR_CE_B_848                    false

#define RFAL_SUPPORT_BR_CE_F_212                    true
#define RFAL_SUPPORT_BR_CE_F_424                    true

typedef enum
{
    RFAL_WUM_PERIOD_10MS      = 0x00,
    RFAL_WUM_PERIOD_20MS      = 0x01,
    RFAL_WUM_PERIOD_30MS      = 0x02,
    RFAL_WUM_PERIOD_40MS      = 0x03,
    RFAL_WUM_PERIOD_50MS      = 0x04,
    RFAL_WUM_PERIOD_60MS      = 0x05,
    RFAL_WUM_PERIOD_70MS      = 0x06,
    RFAL_WUM_PERIOD_80MS      = 0x07,
    RFAL_WUM_PERIOD_100MS     = 0x10,
    RFAL_WUM_PERIOD_200MS     = 0x11,
    RFAL_WUM_PERIOD_300MS     = 0x12,
    RFAL_WUM_PERIOD_400MS     = 0x13,
    RFAL_WUM_PERIOD_500MS     = 0x14,
    RFAL_WUM_PERIOD_600MS     = 0x15,
    RFAL_WUM_PERIOD_700MS     = 0x16,
    RFAL_WUM_PERIOD_800MS     = 0x17,
} rfalWumPeriod;

typedef enum
{
    RFAL_WUM_AA_WEIGHT_4       = 0x00,
    RFAL_WUM_AA_WEIGHT_8       = 0x01,
    RFAL_WUM_AA_WEIGHT_16      = 0x02,
    RFAL_WUM_AA_WEIGHT_32      = 0x03,
} rfalWumAAWeight;

typedef struct
{
    rfalWumPeriod        period;
    bool                 irqTout;
    bool                 swTagDetect;

    struct{
        bool             enabled;
        rfalWumPeriod    refDelay;
    }refWU;

    struct{
        bool             enabled;
        uint8_t          delta;
        uint8_t          fracDelta;
        uint16_t         reference;
        bool             autoAvg;
        bool             aaInclMeas;
        rfalWumAAWeight  aaWeight;
    }indAmp;
    struct{
        bool             enabled;
        uint8_t          delta;
        uint8_t          fracDelta;
        uint16_t         reference;
        bool             autoAvg;
        bool             aaInclMeas;
        rfalWumAAWeight  aaWeight;
    }indPha;
    struct{
        bool             enabled;
        uint8_t          delta;
        uint16_t         reference;
        bool             autoAvg;
        bool             aaInclMeas;
        rfalWumAAWeight  aaWeight;
    }cap;
} rfalWakeUpConfig;

typedef struct
{
    bool                 irqWut;
    struct{
        uint8_t          lastMeas;
        uint16_t         reference;
        bool             irqWu;
    }indAmp;
    struct{
        uint8_t          lastMeas;
        uint16_t         reference;
        bool             irqWu;
    }indPha;
    struct{
        uint8_t          lastMeas;
        uint16_t         reference;
        bool             irqWu;
    }cap;
} rfalWakeUpInfo;

#endif
