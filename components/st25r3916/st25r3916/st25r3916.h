#ifndef ST25R3916_H
#define ST25R3916_H

#include "rfal_platform.h"
#include "rfal_utils.h"
#include "st25r3916_com.h"

typedef struct{
    uint8_t RsA[(ST25R3916_REG_IC_IDENTITY+1U)];
    uint8_t RsB[ST25R3916_SPACE_B_REG_LEN];
}t_st25r3916Regs;

struct st25r3916StreamConfig {
    uint8_t useBPSK;
    uint8_t din;
    uint8_t dout;
    uint8_t report_period_length;
};

#define ST25R3916_CMD_SET_DEFAULT              0xC1U
#define ST25R3916_CMD_STOP                     0xC2U
#define ST25R3916_CMD_TRANSMIT_WITH_CRC        0xC4U
#define ST25R3916_CMD_TRANSMIT_WITHOUT_CRC     0xC5U
#define ST25R3916_CMD_TRANSMIT_REQA            0xC6U
#define ST25R3916_CMD_TRANSMIT_WUPA            0xC7U
#define ST25R3916_CMD_INITIAL_RF_COLLISION     0xC8U
#define ST25R3916_CMD_RESPONSE_RF_COLLISION_N  0xC9U
#define ST25R3916_CMD_GOTO_SENSE               0xCDU
#define ST25R3916_CMD_GOTO_SLEEP               0xCEU
#define ST25R3916_CMD_MASK_RECEIVE_DATA        0xD0U
#define ST25R3916_CMD_UNMASK_RECEIVE_DATA      0xD1U
#define ST25R3916_CMD_AM_MOD_STATE_CHANGE      0xD2U
#define ST25R3916_CMD_MEASURE_AMPLITUDE        0xD3U
#define ST25R3916_CMD_RESET_RXGAIN             0xD5U
#define ST25R3916_CMD_ADJUST_REGULATORS        0xD6U
#define ST25R3916_CMD_CALIBRATE_DRIVER_TIMING  0xD8U
#define ST25R3916_CMD_MEASURE_PHASE            0xD9U
#define ST25R3916_CMD_CLEAR_RSSI               0xDAU
#define ST25R3916_CMD_CLEAR_FIFO               0xDBU
#define ST25R3916_CMD_TRANSPARENT_MODE         0xDCU
#ifdef ST25R3916
#define ST25R3916_CMD_CALIBRATE_C_SENSOR       0xDDU
#define ST25R3916_CMD_MEASURE_CAPACITANCE      0xDEU
#endif
#define ST25R3916_CMD_MEASURE_VDD              0xDFU
#define ST25R3916_CMD_START_GP_TIMER           0xE0U
#define ST25R3916_CMD_START_WUP_TIMER          0xE1U
#define ST25R3916_CMD_START_MASK_RECEIVE_TIMER 0xE2U
#define ST25R3916_CMD_START_NO_RESPONSE_TIMER  0xE3U
#define ST25R3916_CMD_START_PPON2_TIMER        0xE4U
#define ST25R3916_CMD_STOP_NRT                 0xE8U
#ifdef ST25R3916B
#define ST25R3916_CMD_RC_CAL                   0xEAU
#endif
#define ST25R3916_CMD_SPACE_B_ACCESS           0xFBU
#define ST25R3916_CMD_TEST_ACCESS              0xFCU

#define ST25R3916_THRESHOLD_DO_NOT_SET         0xFFU

#define ST25R3916_BR_DO_NOT_SET                0xFFU
#define ST25R3916_BR_106                       0x00U
#define ST25R3916_BR_212                       0x01U
#define ST25R3916_BR_424                       0x02U
#define ST25R3916_BR_848                       0x03U
#define ST25R3916_BR_1695                      0x04U
#define ST25R3916_BR_3390                      0x05U
#define ST25R3916_BR_6780                      0x07U

#define ST25R3916_FIFO_DEPTH                   512U
#define ST25R3916_TOUT_OSC_STABLE              10U

#define st25r3916TxRxOn()             st25r3916SetRegisterBits( ST25R3916_REG_OP_CONTROL, (ST25R3916_REG_OP_CONTROL_rx_en | ST25R3916_REG_OP_CONTROL_tx_en ) )

#define st25r3916TxRxOff()            st25r3916ClrRegisterBits( ST25R3916_REG_OP_CONTROL, (ST25R3916_REG_OP_CONTROL_rx_en | ST25R3916_REG_OP_CONTROL_tx_en ) )

#define st25r3916TxOff()              st25r3916ClrRegisterBits( ST25R3916_REG_OP_CONTROL, ST25R3916_REG_OP_CONTROL_tx_en )

#define st25r3916IsGPTRunning( )      st25r3916CheckReg( ST25R3916_REG_NFCIP1_BIT_RATE, ST25R3916_REG_NFCIP1_BIT_RATE_gpt_on, ST25R3916_REG_NFCIP1_BIT_RATE_gpt_on )

#define st25r3916IsExtFieldOn()       st25r3916CheckReg( ST25R3916_REG_AUX_DISPLAY, ST25R3916_REG_AUX_DISPLAY_efd_o, ST25R3916_REG_AUX_DISPLAY_efd_o )

#define st25r3916IsTxEnabled()        st25r3916CheckReg( ST25R3916_REG_OP_CONTROL, ST25R3916_REG_OP_CONTROL_tx_en, ST25R3916_REG_OP_CONTROL_tx_en )

#define st25r3916IsNRTinEMV()         st25r3916CheckReg( ST25R3916_REG_TIMER_EMV_CONTROL, ST25R3916_REG_TIMER_EMV_CONTROL_nrt_emv, ST25R3916_REG_TIMER_EMV_CONTROL_nrt_emv_on )

#define st25r3916IsLastFIFOComplete() st25r3916CheckReg( ST25R3916_REG_FIFO_STATUS2, ST25R3916_REG_FIFO_STATUS2_fifo_lb_mask, 0 )

#define st25r3916IsOscOn()            st25r3916CheckReg( ST25R3916_REG_OP_CONTROL, ST25R3916_REG_OP_CONTROL_en, ST25R3916_REG_OP_CONTROL_en )

#define st25r3916IsAATOn()            st25r3916CheckReg( ST25R3916_REG_IO_CONF2, ST25R3916_REG_IO_CONF2_aat_en, ST25R3916_REG_IO_CONF2_aat_en )

ReturnCode st25r3916Initialize( void );

void st25r3916Deinitialize( void );

ReturnCode st25r3916OscOn( void );

ReturnCode st25r3916SetBitrate( uint8_t txrate, uint8_t rxrate );

ReturnCode st25r3916AdjustRegulators( uint16_t* result_mV );

ReturnCode st25r3916MeasureAmplitude( uint8_t* result );

uint8_t st25r3916MeasurePowerSupply( uint8_t mpsv );

uint16_t st25r3916MeasureVoltage( uint8_t mpsv );

ReturnCode st25r3916MeasurePhase( uint8_t* result );

ReturnCode st25r3916MeasureCapacitance( uint8_t* result );

ReturnCode st25r3916CalibrateCapacitiveSensor( uint8_t* result );

uint32_t st25r3916GetNoResponseTime( void );

ReturnCode st25r3916SetNoResponseTime( uint32_t nrt_64fcs );

ReturnCode st25r3916SetStartNoResponseTimer( uint32_t nrt_64fcs );

void st25r3916SetGPTime( uint16_t gpt_8fcs );

ReturnCode st25r3916SetStartGPTimer( uint16_t gpt_8fcs, uint8_t trigger_source );

void st25r3916SetNumTxBits( uint16_t nBits );

uint16_t st25r3916GetNumFIFOBytes( void );

uint8_t st25r3916GetNumFIFOLastBits( void );

ReturnCode st25r3916PerformCollisionAvoidance( uint8_t FieldONCmd, uint8_t pdThreshold, uint8_t caThreshold, uint8_t nTRFW );

bool st25r3916CheckChipID( uint8_t *rev );

ReturnCode st25r3916GetRegsDump( t_st25r3916Regs* regDump );

bool st25r3916IsCmdValid( uint8_t cmd );

ReturnCode st25r3916StreamConfigure( const struct st25r3916StreamConfig *config );

ReturnCode st25r3916ExecuteCommandAndGetResult( uint8_t cmd, uint8_t resReg, uint8_t tOut, uint8_t* result );

ReturnCode st25r3916GetRSSI( uint16_t *amRssi, uint16_t *pmRssi );

ReturnCode st25r3916SetAntennaMode( bool single, bool rfiox );

#endif

