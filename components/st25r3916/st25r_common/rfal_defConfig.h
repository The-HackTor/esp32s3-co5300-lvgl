#ifndef RFAL_CONFIG_H
#define RFAL_CONFIG_H

#include "rfal_features.h"

#ifndef RFAL_FEATURE_LISTEN_MODE
    #if RFAL_SUPPORT_CE || RFAL_SUPPORT_MODE_LISTEN_ACTIVE_P2P
        #define RFAL_FEATURE_LISTEN_MODE            true
    #endif
#endif

#ifndef RFAL_FEATURE_WAKEUP_MODE
    #define RFAL_FEATURE_WAKEUP_MODE                true
#endif

#ifndef RFAL_FEATURE_LOWPOWER_MODE
    #define RFAL_FEATURE_LOWPOWER_MODE              false
#endif

#ifndef RFAL_FEATURE_NFCA
    #if RFAL_SUPPORT_MODE_POLL_NFCA
        #define RFAL_FEATURE_NFCA                   true
    #endif
#endif

#ifndef RFAL_FEATURE_T1T
    #if RFAL_SUPPORT_MODE_POLL_NFCA
        #define RFAL_FEATURE_T1T                    true
    #endif
#endif

#ifndef RFAL_FEATURE_T2T
    #if RFAL_SUPPORT_MODE_POLL_NFCA
        #define RFAL_FEATURE_T2T                    true
    #endif
#endif

#ifndef RFAL_FEATURE_T4T
    #if RFAL_SUPPORT_MODE_POLL_NFCA
        #define RFAL_FEATURE_T4T                    true
    #endif
#endif

#ifndef RFAL_FEATURE_NFCB
    #if RFAL_SUPPORT_MODE_POLL_NFCB
        #define RFAL_FEATURE_NFCB                   true
    #endif
#endif

#ifndef RFAL_FEATURE_ST25TB
    #if RFAL_SUPPORT_MODE_POLL_NFCB
        #define RFAL_FEATURE_ST25TB                 true
    #endif
#endif

#ifndef RFAL_FEATURE_NFCF
    #if RFAL_SUPPORT_MODE_POLL_NFCF
        #define RFAL_FEATURE_NFCF                   true
    #endif
#endif

#ifndef RFAL_FEATURE_NFCV
    #if RFAL_SUPPORT_MODE_POLL_NFCV
        #define RFAL_FEATURE_NFCV                   true
    #endif
#endif

#ifndef RFAL_FEATURE_ISO_DEP
    #if RFAL_SUPPORT_MODE_POLL_NFCA || RFAL_SUPPORT_MODE_POLL_NFCB || RFAL_SUPPORT_CE
        #define RFAL_FEATURE_ISO_DEP                true
    #endif
#endif

#ifndef RFAL_FEATURE_ISO_DEP_POLL
    #if RFAL_SUPPORT_MODE_POLL_NFCA || RFAL_SUPPORT_MODE_POLL_NFCB
        #define RFAL_FEATURE_ISO_DEP_POLL           true
    #endif
#endif

#ifndef RFAL_FEATURE_ISO_DEP_LISTEN
    #if RFAL_SUPPORT_CE
        #define RFAL_FEATURE_ISO_DEP_LISTEN         true
    #endif
#endif

#ifndef RFAL_FEATURE_ISO_DEP_IBLOCK_MAX_LEN
    #if RFAL_FEATURE_ISO_DEP
        #define RFAL_FEATURE_ISO_DEP_IBLOCK_MAX_LEN 256U
    #endif
#endif

#ifndef RFAL_FEATURE_ISO_DEP_APDU_MAX_LEN
    #if RFAL_FEATURE_ISO_DEP
        #define RFAL_FEATURE_ISO_DEP_APDU_MAX_LEN   512U
    #endif
#endif

#ifndef RFAL_FEATURE_NFC_DEP
    #if RFAL_SUPPORT_MODE_POLL_NFCA && RFAL_SUPPORT_MODE_POLL_NFCF
        #define RFAL_FEATURE_NFC_DEP                true
    #endif
#endif

#ifndef RFAL_FEATURE_NFC_DEP_BLOCK_MAX_LEN
    #if RFAL_FEATURE_NFC_DEP
        #define RFAL_FEATURE_NFC_DEP_BLOCK_MAX_LEN  254U
    #endif
#endif

#ifndef RFAL_FEATURE_NFC_DEP_PDU_MAX_LEN
    #if RFAL_FEATURE_NFC_DEP
        #define RFAL_FEATURE_NFC_DEP_PDU_MAX_LEN    512U
    #endif
#endif

#ifndef RFAL_FEATURE_NFC_RF_BUF_LEN
    #define RFAL_FEATURE_NFC_RF_BUF_LEN             258U
#endif

#ifndef RFAL_FEATURE_ST25xV
    #define RFAL_FEATURE_ST25xV                     false
#endif

#ifndef RFAL_FEATURE_DYNAMIC_ANALOG_CONFIG
    #define RFAL_FEATURE_DYNAMIC_ANALOG_CONFIG      false
#endif

#ifndef RFAL_FEATURE_DPO
    #define RFAL_FEATURE_DPO                        false
#endif

#ifndef RFAL_FEATURE_DLMA
    #define RFAL_FEATURE_DLMA                       false
#endif

#ifndef platformProtectST25RIrqStatus
    #define platformProtectST25RIrqStatus()
#endif

#ifndef platformUnprotectST25RIrqStatus
    #define platformUnprotectST25RIrqStatus()
#endif

#ifndef platformProtectWorker
    #define platformProtectWorker()
#endif

#ifndef platformUnprotectWorker
    #define platformUnprotectWorker()
#endif

#ifndef platformIrqST25RPinInitialize
    #define platformIrqST25RPinInitialize()
#endif

#ifndef platformIrqST25RSetCallback
    #define platformIrqST25RSetCallback( cb )
#endif

#ifndef platformLedsInitialize
    #define platformLedsInitialize()
#endif

#ifndef platformLedOff
    #define platformLedOff( port, pin )
#endif

#ifndef platformLedOn
    #define platformLedOn( port, pin )
#endif

#ifndef platformLedToggle
    #define platformLedToggle( port, pin )
#endif

#ifndef platformGetSysTick
    #define platformGetSysTick()
#endif

#ifndef platformTimerDestroy
    #define platformTimerDestroy( timer )
#endif

#ifndef platformLog
    #define platformLog(...)
#endif

#ifndef platformAssert
    #define platformAssert( exp )
#endif

#ifndef platformErrorHandle
    #define platformErrorHandle()
#endif

#ifdef RFAL_USE_I2C

    #ifndef platformSpiTxRx
        #define platformSpiTxRx( txBuf, rxBuf, len )
    #endif

#else

    #ifndef platformI2CTx
        #define platformI2CTx( txBuf, len, last, txOnly )
    #endif

    #ifndef platformI2CRx
        #define platformI2CRx( txBuf, len )
    #endif

    #ifndef platformI2CStart
        #define platformI2CStart()
    #endif

    #ifndef platformI2CStop
        #define platformI2CStop()
    #endif

    #ifndef platformI2CRepeatStart
        #define platformI2CRepeatStart()
    #endif

    #ifndef platformI2CSlaveAddrWR
        #define platformI2CSlaveAddrWR(add)
    #endif

    #ifndef platformI2CSlaveAddrRD
        #define platformI2CSlaveAddrRD(add)
    #endif

#endif

#endif

