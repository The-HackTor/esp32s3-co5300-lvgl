#ifndef RFAL_RF_H
#define RFAL_RF_H

#include "rfal_platform.h"
#include "rfal_utils.h"
#include "rfal_features.h"

#define RFAL_VERSION                               0x030001U

#define RFAL_FWT_NONE                              0xFFFFFFFFU
#define RFAL_GT_NONE                               RFAL_TIMING_NONE

#define RFAL_TIMING_NONE                           0x00U

#define RFAL_1FC_IN_4096FC                         (uint32_t)4096U
#define RFAL_1FC_IN_2048FC                         (uint32_t)2048U
#define RFAL_1FC_IN_512FC                          (uint32_t)512U
#define RFAL_1FC_IN_64FC                           (uint32_t)64U
#define RFAL_1FC_IN_8FC                            (uint32_t)8U
#define RFAL_US_IN_MS                              (uint32_t)1000U
#define RFAL_1MS_IN_1FC                            (uint32_t)13560U
#define RFAL_BITS_IN_BYTE                          (uint16_t)8U

#define RFAL_CRC_LEN                               2U

#define RFAL_TXRX_FLAGS_DEFAULT                    ( (uint32_t)RFAL_TXRX_FLAGS_CRC_TX_AUTO | (uint32_t)RFAL_TXRX_FLAGS_CRC_RX_REMV | (uint32_t)RFAL_TXRX_FLAGS_NFCIP1_OFF | (uint32_t)RFAL_TXRX_FLAGS_AGC_ON | (uint32_t)RFAL_TXRX_FLAGS_PAR_RX_REMV | (uint32_t)RFAL_TXRX_FLAGS_PAR_TX_AUTO | (uint32_t)RFAL_TXRX_FLAGS_NFCV_FLAG_AUTO)

#define RFAL_LM_MASK_NFCA                          ((uint32_t)1U<<(uint8_t)RFAL_MODE_LISTEN_NFCA)
#define RFAL_LM_MASK_NFCB                          ((uint32_t)1U<<(uint8_t)RFAL_MODE_LISTEN_NFCB)
#define RFAL_LM_MASK_NFCF                          ((uint32_t)1U<<(uint8_t)RFAL_MODE_LISTEN_NFCF)
#define RFAL_LM_MASK_ACTIVE_P2P                    ((uint32_t)1U<<(uint8_t)RFAL_MODE_LISTEN_ACTIVE_P2P)

#define RFAL_LM_SENS_RES_LEN                       2U
#define RFAL_LM_SENSB_RES_LEN                      13U
#define RFAL_LM_SENSF_RES_LEN                      19U
#define RFAL_LM_SENSF_SC_LEN                       2U

#define RFAL_NFCID3_LEN                            10U
#define RFAL_NFCID2_LEN                            8U
#define RFAL_NFCID1_TRIPLE_LEN                     10U
#define RFAL_NFCID1_DOUBLE_LEN                     7U
#define RFAL_NFCID1_SINGLE_LEN                     4U

#define rfalGetMaxBrRW()                     ( ((RFAL_SUPPORT_BR_RW_6780)  ? RFAL_BR_6780 : ((RFAL_SUPPORT_BR_RW_3390)  ? RFAL_BR_3390 : ((RFAL_SUPPORT_BR_RW_1695)  ? RFAL_BR_1695 : ((RFAL_SUPPORT_BR_RW_848)  ? RFAL_BR_848 : ((RFAL_SUPPORT_BR_RW_424)  ? RFAL_BR_424 : ((RFAL_SUPPORT_BR_RW_212)  ? RFAL_BR_212 : RFAL_BR_106 ) ) ) ) ) ) )

#define rfalGetMaxBrAP2P()                   ( ((RFAL_SUPPORT_BR_AP2P_848) ? RFAL_BR_848  : ((RFAL_SUPPORT_BR_AP2P_424) ? RFAL_BR_424  : ((RFAL_SUPPORT_BR_AP2P_212) ? RFAL_BR_212  : RFAL_BR_106 ) ) ) )

#define rfalGetMaxBrCEA()                    ( ((RFAL_SUPPORT_BR_CE_A_848) ? RFAL_BR_848  : ((RFAL_SUPPORT_BR_CE_A_424) ? RFAL_BR_424  : ((RFAL_SUPPORT_BR_CE_A_212) ? RFAL_BR_212  : RFAL_BR_106 ) ) ) )

#define rfalGetMaxBrCEB()                    ( ((RFAL_SUPPORT_BR_CE_B_848) ? RFAL_BR_848  : ((RFAL_SUPPORT_BR_CE_B_424) ? RFAL_BR_424  : ((RFAL_SUPPORT_BR_CE_B_212) ? RFAL_BR_212  : RFAL_BR_106 ) ) ) )

#define rfalGetMaxBrCEF()                    ( ((RFAL_SUPPORT_BR_CE_F_424) ? RFAL_BR_424  : RFAL_BR_212 ) )

#define rfalIsModeActiveComm( md )           ( ((md) == RFAL_MODE_POLL_ACTIVE_P2P) || ((md) == RFAL_MODE_LISTEN_ACTIVE_P2P) )
#define rfalIsModePassiveComm( md )          ( !rfalIsModeActiveComm(md) )
#define rfalIsModePassiveListen( md )        ( ((md) == RFAL_MODE_LISTEN_NFCA) || ((md) == RFAL_MODE_LISTEN_NFCB) || ((md) == RFAL_MODE_LISTEN_NFCF) )
#define rfalIsModePassivePoll( md )          ( rfalIsModePassiveComm(md) && (!rfalIsModePassiveListen(md)) )

#define rfalConv1fcTo8fc( t )                (uint32_t)( (uint32_t)(t) / RFAL_1FC_IN_8FC )
#define rfalConv8fcTo1fc( t )                (uint32_t)( (uint32_t)(t) * RFAL_1FC_IN_8FC )

#define rfalConv1fcTo64fc( t )               (uint32_t)( (uint32_t)(t) / RFAL_1FC_IN_64FC )
#define rfalConv64fcTo1fc( t )               (uint32_t)( (uint32_t)(t) * RFAL_1FC_IN_64FC )

#define rfalConv1fcTo512fc( t )              (uint32_t)( (uint32_t)(t) / RFAL_1FC_IN_512FC )
#define rfalConv512fcTo1fc( t )              (uint32_t)( (uint32_t)(t) * RFAL_1FC_IN_512FC )

#define rfalConv1fcTo2018fc( t )             (uint32_t)( (uint32_t)(t) / RFAL_1FC_IN_2048FC )
#define rfalConv2048fcTo1fc( t )             (uint32_t)( (uint32_t)(t) * RFAL_1FC_IN_2048FC )

#define rfalConv1fcTo4096fc( t )             (uint32_t)( (uint32_t)(t) / RFAL_1FC_IN_4096FC )
#define rfalConv4096fcTo1fc( t )             (uint32_t)( (uint32_t)(t) * RFAL_1FC_IN_4096FC )

#define rfalConv1fcToMs( t )                 (uint32_t)( (uint32_t)(t) / RFAL_1MS_IN_1FC )
#define rfalConvMsTo1fc( t )                 (uint32_t)( (uint32_t)(t) * RFAL_1MS_IN_1FC )

#define rfalConv1fcToUs( t )                 (uint32_t)( ((uint32_t)(t) * RFAL_US_IN_MS) / RFAL_1MS_IN_1FC)
#define rfalConvUsTo1fc( t )                 (uint32_t)( ((uint32_t)(t) * RFAL_1MS_IN_1FC) / RFAL_US_IN_MS)

#define rfalConv64fcToMs( t )                (uint32_t)( (uint32_t)(t) / (RFAL_1MS_IN_1FC / RFAL_1FC_IN_64FC) )
#define rfalConvMsTo64fc( t )                (uint32_t)( (uint32_t)(t) * (RFAL_1MS_IN_1FC / RFAL_1FC_IN_64FC) )

#define rfalConvBitsToBytes( n )             (uint16_t)( ((uint16_t)(n)+(RFAL_BITS_IN_BYTE-1U)) / (RFAL_BITS_IN_BYTE) )
#define rfalConvBytesToBits( n )             (uint32_t)( (uint32_t)(n) * (RFAL_BITS_IN_BYTE) )

#define rfalRunBlocking( e, fn )              do{ (e)=(fn); rfalWorker(); }while( (e) == RFAL_ERR_BUSY )

#define rfalCreateByteTxRxContext( ctx, tB, tBL, rB, rBL, rdL, t ) \
    (ctx).txBuf     = (uint8_t*)(tB);                                      \
    (ctx).txBufLen  = (uint16_t)rfalConvBytesToBits(tBL);                  \
    (ctx).rxBuf     = (uint8_t*)(rB);                                      \
    (ctx).rxBufLen  = (uint16_t)rfalConvBytesToBits(rBL);                  \
    (ctx).rxRcvdLen = (uint16_t*)(rdL);                                    \
    (ctx).flags     = (uint32_t)RFAL_TXRX_FLAGS_DEFAULT;                   \
    (ctx).fwt       = (uint32_t)(t);

#define rfalCreateByteFlagsTxRxContext( ctx, tB, tBL, rB, rBL, rdL, fl, t ) \
    (ctx).txBuf     = (uint8_t*)(tB);                                       \
    (ctx).txBufLen  = (uint16_t)rfalConvBytesToBits(tBL);                   \
    (ctx).rxBuf     = (uint8_t*)(rB);                                       \
    (ctx).rxBufLen  = (uint16_t)rfalConvBytesToBits(rBL);                   \
    (ctx).rxRcvdLen = (uint16_t*)(rdL);                                     \
    (ctx).flags     = (uint32_t)(fl);                                       \
    (ctx).fwt       = (uint32_t)(t);

#define rfalLogE(...)             platformLog(__VA_ARGS__)
#define rfalLogW(...)             platformLog(__VA_ARGS__)
#define rfalLogI(...)             platformLog(__VA_ARGS__)
#define rfalLogD(...)             platformLog(__VA_ARGS__)

#define    RFAL_GT_NFCA                      rfalConvMsTo1fc(5U)
#define    RFAL_GT_NFCB                      rfalConvMsTo1fc(5U)
#define    RFAL_GT_NFCF                      rfalConvMsTo1fc(20U)
#define    RFAL_GT_NFCV                      rfalConvMsTo1fc(5U)
#define    RFAL_GT_PICOPASS                  rfalConvMsTo1fc(1U)
#define    RFAL_GT_AP2P                      rfalConvMsTo1fc(5U)
#define    RFAL_GT_AP2P_ADJUSTED             rfalConvMsTo1fc(5U+25U)

#define    RFAL_FDT_LISTEN_NFCA_POLLER       1172U
#define    RFAL_FDT_LISTEN_NFCB_POLLER       1008U
#define    RFAL_FDT_LISTEN_NFCF_POLLER       2672U
#define    RFAL_FDT_LISTEN_NFCV_POLLER       4310U
#define    RFAL_FDT_LISTEN_PICOPASS_POLLER   3400U
#define    RFAL_FDT_LISTEN_AP2P_POLLER       64U
#define    RFAL_FDT_LISTEN_NFCA_LISTENER     1172U
#define    RFAL_FDT_LISTEN_NFCB_LISTENER     1024U
#define    RFAL_FDT_LISTEN_NFCF_LISTENER     2688U
#define    RFAL_FDT_LISTEN_AP2P_LISTENER     64U

#define    RFAL_FDT_POLL_NFCA_POLLER         6780U
#define    RFAL_FDT_POLL_NFCA_T1T_POLLER     384U
#define    RFAL_FDT_POLL_NFCB_POLLER         6780U
#define    RFAL_FDT_POLL_NFCF_POLLER         6800U
#define    RFAL_FDT_POLL_NFCV_POLLER         4192U
#define    RFAL_FDT_POLL_PICOPASS_POLLER     1790U
#define    RFAL_FDT_POLL_AP2P_POLLER         6800U

typedef enum {
    RFAL_MODE_NONE                   = 0,
    RFAL_MODE_POLL_NFCA              = 1,
    RFAL_MODE_POLL_NFCA_T1T          = 2,
    RFAL_MODE_POLL_NFCB              = 3,
    RFAL_MODE_POLL_B_PRIME           = 4,
    RFAL_MODE_POLL_B_CTS             = 5,
    RFAL_MODE_POLL_NFCF              = 6,
    RFAL_MODE_POLL_NFCV              = 7,
    RFAL_MODE_POLL_PICOPASS          = 8,
    RFAL_MODE_POLL_ACTIVE_P2P        = 9,
    RFAL_MODE_LISTEN_NFCA            = 10,
    RFAL_MODE_LISTEN_NFCB            = 11,
    RFAL_MODE_LISTEN_NFCF            = 12,
    RFAL_MODE_LISTEN_ACTIVE_P2P      = 13
} rfalMode;

typedef enum {
    RFAL_BR_106                      = 0,
    RFAL_BR_212                      = 1,
    RFAL_BR_424                      = 2,
    RFAL_BR_848                      = 3,
    RFAL_BR_1695                     = 4,
    RFAL_BR_3390                     = 5,
    RFAL_BR_6780                     = 6,
    RFAL_BR_13560                    = 7,
    RFAL_BR_211p88                   = 0xE9,
    RFAL_BR_105p94                   = 0xEA,
    RFAL_BR_52p97                    = 0xEB,
    RFAL_BR_26p48                    = 0xEC,
    RFAL_BR_1p66                     = 0xED,
    RFAL_BR_KEEP                     = 0xFF
} rfalBitRate;

typedef enum {
    RFAL_COMPLIANCE_MODE_NFC,
    RFAL_COMPLIANCE_MODE_EMV,
    RFAL_COMPLIANCE_MODE_ISO
}rfalComplianceMode;

typedef enum {
    RFAL_STATE_IDLE                  = 0,
    RFAL_STATE_INIT                  = 1,
    RFAL_STATE_MODE_SET              = 2,

    RFAL_STATE_TXRX                  = 3,
    RFAL_STATE_LM                    = 4,
    RFAL_STATE_WUM                   = 5

} rfalState;

typedef enum {
    RFAL_TXRX_STATE_IDLE             = 0,
    RFAL_TXRX_STATE_INIT             = 1,
    RFAL_TXRX_STATE_START            = 2,

    RFAL_TXRX_STATE_TX_IDLE          = 11,
    RFAL_TXRX_STATE_TX_WAIT_GT       = 12,
    RFAL_TXRX_STATE_TX_WAIT_FDT      = 13,
    RFAL_TXRX_STATE_TX_PREP_TX       = 14,
    RFAL_TXRX_STATE_TX_TRANSMIT      = 15,
    RFAL_TXRX_STATE_TX_WAIT_WL       = 16,
    RFAL_TXRX_STATE_TX_RELOAD_FIFO   = 17,
    RFAL_TXRX_STATE_TX_WAIT_TXE      = 18,
    RFAL_TXRX_STATE_TX_DONE          = 19,
    RFAL_TXRX_STATE_TX_FAIL          = 20,

    RFAL_TXRX_STATE_RX_IDLE          = 81,
    RFAL_TXRX_STATE_RX_WAIT_EON      = 82,
    RFAL_TXRX_STATE_RX_WAIT_RXS      = 83,
    RFAL_TXRX_STATE_RX_WAIT_RXE      = 84,
    RFAL_TXRX_STATE_RX_READ_FIFO     = 85,
    RFAL_TXRX_STATE_RX_ERR_CHECK     = 86,
    RFAL_TXRX_STATE_RX_READ_DATA     = 87,
    RFAL_TXRX_STATE_RX_WAIT_EOF      = 88,
    RFAL_TXRX_STATE_RX_DONE          = 89,
    RFAL_TXRX_STATE_RX_FAIL          = 90,

} rfalTransceiveState;

enum {
    RFAL_TXRX_FLAGS_CRC_TX_AUTO      = (0U<<0),
    RFAL_TXRX_FLAGS_CRC_TX_MANUAL    = (1U<<0),
    RFAL_TXRX_FLAGS_CRC_RX_KEEP      = (1U<<1),
    RFAL_TXRX_FLAGS_CRC_RX_REMV      = (0U<<1),
    RFAL_TXRX_FLAGS_NFCIP1_ON        = (1U<<2),
    RFAL_TXRX_FLAGS_NFCIP1_OFF       = (0U<<2),
    RFAL_TXRX_FLAGS_AGC_OFF          = (1U<<3),
    RFAL_TXRX_FLAGS_AGC_ON           = (0U<<3),
    RFAL_TXRX_FLAGS_PAR_RX_KEEP      = (1U<<4),
    RFAL_TXRX_FLAGS_PAR_RX_REMV      = (0U<<4),
    RFAL_TXRX_FLAGS_PAR_TX_NONE      = (1U<<5),
    RFAL_TXRX_FLAGS_PAR_TX_AUTO      = (0U<<5),
    RFAL_TXRX_FLAGS_NFCV_FLAG_MANUAL = (1U<<6),
    RFAL_TXRX_FLAGS_NFCV_FLAG_AUTO   = (0U<<6),
    RFAL_TXRX_FLAGS_CRC_RX_MANUAL    = (1U<<7),
    RFAL_TXRX_FLAGS_CRC_RX_AUTO      = (0U<<7),
};

typedef enum {
    RFAL_ERRORHANDLING_NONE          = 0,
    RFAL_ERRORHANDLING_EMD           = 1
} rfalEHandling;

typedef struct {
    uint8_t*              txBuf;
    uint16_t              txBufLen;

    uint8_t*              rxBuf;
    uint16_t              rxBufLen;
    uint16_t*             rxRcvdLen;

    uint32_t              flags;
    uint32_t              fwt;
} rfalTransceiveContext;

typedef void (* rfalUpperLayerCallback)(void);

typedef void (* rfalPreTxRxCallback)(void);

typedef void (* rfalPostTxRxCallback)(void);

typedef bool (* rfalSyncTxRxCallback)(void);

typedef void (* rfalLmEonCallback)(void);

typedef enum
{
     RFAL_14443A_SHORTFRAME_CMD_WUPA = 0x52,
     RFAL_14443A_SHORTFRAME_CMD_REQA = 0x26
} rfal14443AShortFrameCmd;

#define RFAL_FELICA_LEN_LEN                        1U
#define RFAL_FELICA_POLL_REQ_LEN                   (RFAL_FELICA_LEN_LEN + 1U + 2U + 1U + 1U)
#define RFAL_FELICA_POLL_RES_LEN                   (RFAL_FELICA_LEN_LEN + 1U + 8U + 8U + 2U)
#define RFAL_FELICA_POLL_MAX_SLOTS                 16U

enum
{
    RFAL_FELICA_POLL_RC_NO_REQUEST        =     0x00U,
    RFAL_FELICA_POLL_RC_SYSTEM_CODE       =     0x01U,
    RFAL_FELICA_POLL_RC_COM_PERFORMANCE   =     0x02U
};

typedef enum
{
    RFAL_FELICA_1_SLOT    =  0,
    RFAL_FELICA_2_SLOTS   =  1,
    RFAL_FELICA_4_SLOTS   =  3,
    RFAL_FELICA_8_SLOTS   =  7,
    RFAL_FELICA_16_SLOTS  =  15
} rfalFeliCaPollSlots;

typedef uint8_t rfalFeliCaPollRes[RFAL_FELICA_POLL_RES_LEN];

typedef enum
{
    RFAL_LM_NFCID_LEN_04  = RFAL_NFCID1_SINGLE_LEN,
    RFAL_LM_NFCID_LEN_07  = RFAL_NFCID1_DOUBLE_LEN,
    RFAL_LM_NFCID_LEN_10  = RFAL_NFCID1_TRIPLE_LEN,
} rfalLmNfcidLen;

typedef enum
{
    RFAL_LM_STATE_NOT_INIT              = 0x00,
    RFAL_LM_STATE_POWER_OFF             = 0x01,
    RFAL_LM_STATE_IDLE                  = 0x02,
    RFAL_LM_STATE_READY_A               = 0x03,
    RFAL_LM_STATE_READY_B               = 0x04,
    RFAL_LM_STATE_READY_F               = 0x05,
    RFAL_LM_STATE_ACTIVE_A              = 0x06,
    RFAL_LM_STATE_CARDEMU_4A            = 0x07,
    RFAL_LM_STATE_CARDEMU_4B            = 0x08,
    RFAL_LM_STATE_CARDEMU_3             = 0x09,
    RFAL_LM_STATE_TARGET_A              = 0x0A,
    RFAL_LM_STATE_TARGET_F              = 0x0B,
    RFAL_LM_STATE_SLEEP_A               = 0x0C,
    RFAL_LM_STATE_SLEEP_B               = 0x0D,
    RFAL_LM_STATE_READY_Ax              = 0x0E,
    RFAL_LM_STATE_ACTIVE_Ax             = 0x0F,
    RFAL_LM_STATE_SLEEP_AF              = 0x10,
} rfalLmState;

typedef struct
{
    rfalLmNfcidLen   nfcidLen;
    uint8_t          nfcid[RFAL_NFCID1_TRIPLE_LEN];
    uint8_t          SENS_RES[RFAL_LM_SENS_RES_LEN];
    uint8_t          SEL_RES;
} rfalLmConfPA;

typedef struct
{
    uint8_t          SENSB_RES[RFAL_LM_SENSB_RES_LEN];
} rfalLmConfPB;

typedef struct
{
    uint8_t          SC[RFAL_LM_SENSF_SC_LEN];
    uint8_t          SENSF_RES[RFAL_LM_SENSF_RES_LEN];
} rfalLmConfPF;

typedef enum {
    RFAL_LP_MODE_PD  = 0,
    RFAL_LP_MODE_HR  = 1
} rfalLpMode;

#define RFAL_WUM_REFERENCE_AUTO           0xFFU

typedef enum
{
    RFAL_WUM_STATE_NOT_INIT              = 0x00,
    RFAL_WUM_STATE_INITIALIZING          = 0x01,
    RFAL_WUM_STATE_ENABLED               = 0x02,
    RFAL_WUM_STATE_ENABLED_WOKE          = 0x03,
} rfalWumState;

ReturnCode rfalInitialize( void );

ReturnCode rfalCalibrate( void );

ReturnCode rfalAdjustRegulators( uint16_t* result );

void rfalSetUpperLayerCallback( rfalUpperLayerCallback pFunc );

void rfalSetPreTxRxCallback( rfalPreTxRxCallback pFunc );

void rfalSetSyncTxRxCallback( rfalSyncTxRxCallback pFunc );

void rfalSetPostTxRxCallback( rfalPostTxRxCallback pFunc );

void rfalSetLmEonCallback( rfalLmEonCallback pFunc );

ReturnCode rfalDeinitialize( void );

ReturnCode rfalSetMode( rfalMode mode, rfalBitRate txBR, rfalBitRate rxBR );

rfalMode rfalGetMode( void );

ReturnCode rfalSetBitRate( rfalBitRate txBR, rfalBitRate rxBR );

ReturnCode rfalGetBitRate( rfalBitRate *txBR, rfalBitRate *rxBR );

void rfalSetErrorHandling( rfalEHandling eHandling );

rfalEHandling rfalGetErrorHandling( void );

void rfalSetObsvMode( uint32_t txMode, uint32_t rxMode );

void rfalGetObsvMode( uint8_t* txMode, uint8_t* rxMode );

void rfalDisableObsvMode( void );

void rfalSetFDTPoll( uint32_t FDTPoll );

uint32_t rfalGetFDTPoll( void );

void rfalSetFDTListen( uint32_t FDTListen );

uint32_t rfalGetFDTListen( void );

uint32_t rfalGetGT( void );

void rfalSetGT( uint32_t GT );

bool rfalIsGTExpired( void );

ReturnCode rfalFieldOnAndStartGT( void );

ReturnCode rfalFieldOff( void );

ReturnCode rfalStartTransceive( const rfalTransceiveContext *ctx );

rfalTransceiveState rfalGetTransceiveState( void );

ReturnCode rfalGetTransceiveStatus( void );

bool rfalIsTransceiveInTx( void );

bool rfalIsTransceiveInRx( void );

ReturnCode rfalGetTransceiveRSSI( uint16_t *rssi );

bool rfalIsTransceiveSubcDetected( void );

void rfalWorker( void );

ReturnCode rfalISO14443ATransceiveShortFrame( rfal14443AShortFrameCmd txCmd, uint8_t* rxBuf, uint8_t rxBufLen, uint16_t* rxRcvdLen, uint32_t fwt );

ReturnCode rfalISO14443ATransceiveAnticollisionFrame( uint8_t *buf, uint8_t *bytesToSend, uint8_t *bitsToSend, uint16_t *rxLength, uint32_t fwt );

ReturnCode rfalISO14443AStartTransceiveAnticollisionFrame( uint8_t *buf, uint8_t *bytesToSend, uint8_t *bitsToSend, uint16_t *rxLength, uint32_t fwt );

ReturnCode rfalISO14443AGetTransceiveAnticollisionFrameStatus( void );

ReturnCode rfalFeliCaPoll( rfalFeliCaPollSlots slots, uint16_t sysCode, uint8_t reqCode, rfalFeliCaPollRes* pollResList, uint8_t pollResListSize, uint8_t *devicesDetected, uint8_t *collisionsDetected );

ReturnCode rfalStartFeliCaPoll( rfalFeliCaPollSlots slots, uint16_t sysCode, uint8_t reqCode, rfalFeliCaPollRes* pollResList, uint8_t pollResListSize, uint8_t *devicesDetected, uint8_t *collisionsDetected );

ReturnCode rfalGetFeliCaPollStatus( void );

ReturnCode rfalISO15693TransceiveAnticollisionFrame( uint8_t *txBuf, uint8_t txBufLen, uint8_t *rxBuf, uint8_t rxBufLen, uint16_t *actLen );

ReturnCode rfalISO15693TransceiveEOFAnticollision( uint8_t *rxBuf, uint8_t rxBufLen, uint16_t *actLen );

ReturnCode rfalISO15693TransceiveEOF( uint8_t *rxBuf, uint16_t rxBufLen, uint16_t *actLen );

ReturnCode rfalTransceiveBlockingTx( uint8_t* txBuf, uint16_t txBufLen, uint8_t* rxBuf, uint16_t rxBufLen, uint16_t* actLen, uint32_t flags, uint32_t fwt );

ReturnCode rfalTransceiveBlockingRx( void );

ReturnCode rfalTransceiveBlockingTxRx( uint8_t* txBuf, uint16_t txBufLen, uint8_t* rxBuf, uint16_t rxBufLen, uint16_t* actLen, uint32_t flags, uint32_t fwt );

bool rfalIsExtFieldOn( void );

ReturnCode rfalListenStart( uint32_t lmMask, const rfalLmConfPA *confA, const rfalLmConfPB *confB, const rfalLmConfPF *confF, uint8_t *rxBuf, uint16_t rxBufLen, uint16_t *rxLen );

ReturnCode rfalListenSleepStart( rfalLmState sleepSt, uint8_t *rxBuf, uint16_t rxBufLen, uint16_t *rxLen );

ReturnCode rfalListenStop( void );

rfalLmState rfalListenGetState( bool *dataFlag, rfalBitRate *lastBR );

ReturnCode rfalListenSetState( rfalLmState newSt );

ReturnCode rfalWakeUpModeStart( const rfalWakeUpConfig *config );

bool rfalWakeUpModeHasWoke( void );

bool rfalWakeUpModeIsEnabled( void );

ReturnCode rfalWakeUpModeGetInfo( bool force, rfalWakeUpInfo *info );

ReturnCode rfalWakeUpModeStop( void );

ReturnCode rfalWlcPWptMonitorStart( const rfalWakeUpConfig *config );

ReturnCode rfalWlcPWptMonitorStop( void );

bool rfalWlcPWptIsFodDetected( void );

bool rfalWlcPWptIsStopDetected( void );

ReturnCode rfalLowPowerModeStart( rfalLpMode mode );

ReturnCode rfalLowPowerModeStop( void );

#endif

