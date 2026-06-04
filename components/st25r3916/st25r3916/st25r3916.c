#include "st25r3916.h"
#include "st25r3916_com.h"
#include "st25r3916_led.h"
#include "st25r3916_irq.h"
#include "rfal_utils.h"

#if !defined(ST25R3916) && !defined(ST25R3916B)
#error "RFAL: Missing ST25R device selection. Please globally define ST25R3916 / ST25R3916B."
#endif

#define ST25R3916_SUPPLY_THRESHOLD                3600U
#define ST25R3916_NRT_MAX                         0xFFFFU

#define ST25R3916_TOUT_MEASURE_VDD                100U
#define ST25R3916_TOUT_MEASURE_AMPLITUDE          10U
#define ST25R3916_TOUT_MEASURE_PHASE              10U
#define ST25R3916_TOUT_MEASURE_CAPACITANCE        10U
#define ST25R3916_TOUT_CALIBRATE_CAP_SENSOR       4U
#define ST25R3916_TOUT_CALIBRATE_AWS_RC           10U
#define ST25R3916_TOUT_ADJUST_REGULATORS          6U
#define ST25R3916_TOUT_CA                         10U

#define ST25R3916_TEST_REG_PATTERN                0x33U
#define ST25R3916_TEST_WU_TOUT                    12U
#define ST25R3916_TEST_TMR_TOUT                   20U
#define ST25R3916_TEST_TMR_TOUT_DELTA             2U
#define ST25R3916_TEST_TMR_TOUT_8FC               (ST25R3916_TEST_TMR_TOUT * 1695U)

static uint32_t gST25R3916NRT_64fcs;

ReturnCode st25r3916Initialize( void )
{
    uint16_t vdd_mV;
    ReturnCode ret;

#ifndef RFAL_USE_I2C

    platformSpiDeselect();
#endif

    st25r3916ExecuteCommand( ST25R3916_CMD_SET_DEFAULT );

#ifndef RFAL_USE_I2C

    st25r3916WriteRegister(ST25R3916_REG_IO_CONF2, ST25R3916_REG_IO_CONF2_io_drv_lvl);
#endif

    if( !st25r3916CheckChipID( NULL ) )
    {
        platformErrorHandle();
        return RFAL_ERR_HW_MISMATCH;
    }

    st25r3916InitInterrupts();
    st25r3916ledInit();

    gST25R3916NRT_64fcs = 0;

#ifndef RFAL_USE_I2C

    st25r3916SetRegisterBits(ST25R3916_REG_IO_CONF2, ( ST25R3916_REG_IO_CONF2_miso_pd1 | ST25R3916_REG_IO_CONF2_miso_pd2 ) );
#endif

#ifdef ST25R3916

    st25r3916ChangeTestRegisterBits( 0x04, 0x10, 0x10 );
#endif

#ifdef ST25R_SELFTEST

    st25r3916WriteRegister( ST25R3916_REG_BIT_RATE, ST25R3916_TEST_REG_PATTERN );
    if( !st25r3916CheckReg( ST25R3916_REG_BIT_RATE, (ST25R3916_REG_BIT_RATE_rxrate_mask | ST25R3916_REG_BIT_RATE_txrate_mask), ST25R3916_TEST_REG_PATTERN ) )
    {
        platformErrorHandle();
        return RFAL_ERR_IO;
    }

    st25r3916WriteRegister( ST25R3916_REG_BIT_RATE, 0x00 );

    st25r3916WriteRegister( ST25R3916_REG_WUP_TIMER_CONTROL, ST25R3916_REG_WUP_TIMER_CONTROL_wur|ST25R3916_REG_WUP_TIMER_CONTROL_wto);
    st25r3916EnableInterrupts( ST25R3916_IRQ_MASK_WT );
    st25r3916ExecuteCommand( ST25R3916_CMD_START_WUP_TIMER );
    if(st25r3916WaitForInterruptsTimed(ST25R3916_IRQ_MASK_WT, ST25R3916_TEST_WU_TOUT) == 0U )
    {
        platformErrorHandle();
        return RFAL_ERR_TIMEOUT;
    }
    st25r3916DisableInterrupts( ST25R3916_IRQ_MASK_WT );
    st25r3916WriteRegister( ST25R3916_REG_WUP_TIMER_CONTROL, 0U );

#endif

    ret = st25r3916OscOn();
    if( ret != RFAL_ERR_NONE )
    {
        platformErrorHandle();
        return ret;
    }

#ifdef ST25R3916B

    st25r3916ExecuteCommandAndGetResult( ST25R3916_CMD_RC_CAL,  ST25R3916_REG_AWS_RC_CAL, ST25R3916_TOUT_CALIBRATE_AWS_RC, NULL );
#endif

    vdd_mV = st25r3916MeasureVoltage( ST25R3916_REG_REGULATOR_CONTROL_mpsv_vdd );
    st25r3916ChangeRegisterBits( ST25R3916_REG_IO_CONF2, ST25R3916_REG_IO_CONF2_sup3V, ((vdd_mV < ST25R3916_SUPPLY_THRESHOLD) ? ST25R3916_REG_IO_CONF2_sup3V_3V : ST25R3916_REG_IO_CONF2_sup3V_5V) );

    st25r3916TxRxOff();

#ifdef ST25R_SELFTEST_TIMER

    st25r3916EnableInterrupts( ST25R3916_IRQ_MASK_GPE );
    st25r3916SetStartGPTimer( (uint16_t)ST25R3916_TEST_TMR_TOUT_8FC, ST25R3916_REG_TIMER_EMV_CONTROL_gptc_no_trigger);
    if( st25r3916WaitForInterruptsTimed( ST25R3916_IRQ_MASK_GPE, (ST25R3916_TEST_TMR_TOUT - ST25R3916_TEST_TMR_TOUT_DELTA)) != 0U )
    {
        platformErrorHandle();
        return RFAL_ERR_SYSTEM;
    }

    st25r3916ExecuteCommand( ST25R3916_CMD_STOP );
    st25r3916ClearAndEnableInterrupts( ST25R3916_IRQ_MASK_GPE );
    st25r3916SetStartGPTimer( (uint16_t)ST25R3916_TEST_TMR_TOUT_8FC, ST25R3916_REG_TIMER_EMV_CONTROL_gptc_no_trigger );
    if(st25r3916WaitForInterruptsTimed( ST25R3916_IRQ_MASK_GPE, (ST25R3916_TEST_TMR_TOUT + ST25R3916_TEST_TMR_TOUT_DELTA)) == 0U )
    {
        platformErrorHandle();
        return RFAL_ERR_SYSTEM;
    }

    st25r3916ExecuteCommand( ST25R3916_CMD_STOP );

#endif

    st25r3916DisableInterrupts( ST25R3916_IRQ_MASK_ALL );

    st25r3916ClearInterrupts();

    return RFAL_ERR_NONE;
}

void st25r3916Deinitialize( void )
{

    st25r3916ExecuteCommand( ST25R3916_CMD_STOP );
    st25r3916DisableInterrupts( ST25R3916_IRQ_MASK_ALL );

    st25r3916ClrRegisterBits( ST25R3916_REG_OP_CONTROL, ( ST25R3916_REG_OP_CONTROL_en | ST25R3916_REG_OP_CONTROL_rx_en |
                                                          ST25R3916_REG_OP_CONTROL_wu | ST25R3916_REG_OP_CONTROL_tx_en | ST25R3916_REG_OP_CONTROL_en_fd_mask ) );

    return;
}

ReturnCode st25r3916OscOn( void )
{

    if( !st25r3916CheckReg( ST25R3916_REG_OP_CONTROL, ST25R3916_REG_OP_CONTROL_en, ST25R3916_REG_OP_CONTROL_en ) )
    {

        st25r3916ClearAndEnableInterrupts( ST25R3916_IRQ_MASK_OSC );

        st25r3916GetInterrupt( ST25R3916_IRQ_MASK_OSC );

        st25r3916SetRegisterBits( ST25R3916_REG_OP_CONTROL, ST25R3916_REG_OP_CONTROL_en );

        st25r3916WaitForInterruptsTimed( ST25R3916_IRQ_MASK_OSC, ST25R3916_TOUT_OSC_STABLE );
        st25r3916DisableInterrupts( ST25R3916_IRQ_MASK_OSC );
    }

    if( !st25r3916CheckReg( ST25R3916_REG_AUX_DISPLAY, ST25R3916_REG_AUX_DISPLAY_osc_ok, ST25R3916_REG_AUX_DISPLAY_osc_ok ) )
    {
        return RFAL_ERR_SYSTEM;
    }

    return RFAL_ERR_NONE;
}

ReturnCode st25r3916ExecuteCommandAndGetResult( uint8_t cmd, uint8_t resReg, uint8_t tOut, uint8_t* result )
{

    st25r3916GetInterrupt( ST25R3916_IRQ_MASK_DCT );
    st25r3916EnableInterrupts( ST25R3916_IRQ_MASK_DCT );

    st25r3916ExecuteCommand( cmd );

    st25r3916WaitForInterruptsTimed( ST25R3916_IRQ_MASK_DCT, tOut );
    st25r3916DisableInterrupts( ST25R3916_IRQ_MASK_DCT );

    if( result != NULL )
    {
        st25r3916ReadRegister( resReg, result);
    }

    return RFAL_ERR_NONE;

}

uint8_t st25r3916MeasurePowerSupply( uint8_t mpsv )
{
    uint8_t result;

    st25r3916ChangeRegisterBits( ST25R3916_REG_REGULATOR_CONTROL, ST25R3916_REG_REGULATOR_CONTROL_mpsv_mask, mpsv );

    st25r3916ExecuteCommandAndGetResult( ST25R3916_CMD_MEASURE_VDD, ST25R3916_REG_AD_RESULT, ST25R3916_TOUT_MEASURE_VDD, &result);

    return result;
}

uint16_t st25r3916MeasureVoltage( uint8_t mpsv )
{
    uint8_t result;
    uint16_t mV;

    result = st25r3916MeasurePowerSupply(mpsv);

    mV  = ((uint16_t)result) * 23U;
    mV += (((((uint16_t)result) * 4U) + 5U) / 10U);

    return mV;
}

ReturnCode st25r3916AdjustRegulators( uint16_t* result_mV )
{
    uint8_t result;

    st25r3916SetRegisterBits( ST25R3916_REG_REGULATOR_CONTROL, ST25R3916_REG_REGULATOR_CONTROL_reg_s );
    st25r3916ClrRegisterBits( ST25R3916_REG_REGULATOR_CONTROL, ST25R3916_REG_REGULATOR_CONTROL_reg_s );

    st25r3916ExecuteCommandAndGetResult( ST25R3916_CMD_ADJUST_REGULATORS, ST25R3916_REG_REGULATOR_RESULT, ST25R3916_TOUT_ADJUST_REGULATORS, &result );

    result >>= ST25R3916_REG_REGULATOR_RESULT_reg_shift;

    if( result_mV != NULL )
    {
        if( st25r3916CheckReg( ST25R3916_REG_IO_CONF2, ST25R3916_REG_IO_CONF2_sup3V, ST25R3916_REG_IO_CONF2_sup3V )  )
        {
            result -= ((result>4U) ? (5U) : 0U);
            *result_mV = 2400U;
        }
        else
        {
            *result_mV = 3600U;
        }

        *result_mV += (uint16_t)result * 100U;
    }
    return RFAL_ERR_NONE;
}

ReturnCode st25r3916MeasureAmplitude( uint8_t* result )
{
    return st25r3916ExecuteCommandAndGetResult( ST25R3916_CMD_MEASURE_AMPLITUDE, ST25R3916_REG_AD_RESULT, ST25R3916_TOUT_MEASURE_AMPLITUDE, result );
}

ReturnCode st25r3916MeasurePhase( uint8_t* result )
{
    return st25r3916ExecuteCommandAndGetResult( ST25R3916_CMD_MEASURE_PHASE, ST25R3916_REG_AD_RESULT, ST25R3916_TOUT_MEASURE_PHASE, result );
}

ReturnCode st25r3916MeasureCapacitance( uint8_t* result )
{
#ifdef ST25R3916B
    return RFAL_ERR_NOTSUPP;
#else
    return st25r3916ExecuteCommandAndGetResult( ST25R3916_CMD_MEASURE_CAPACITANCE, ST25R3916_REG_AD_RESULT, ST25R3916_TOUT_MEASURE_CAPACITANCE, result );
#endif
}

ReturnCode st25r3916CalibrateCapacitiveSensor( uint8_t* result )
{
#ifdef ST25R3916B
    return RFAL_ERR_NOTSUPP;
#else
    ReturnCode ret;
    uint8_t    res;

    st25r3916ClrRegisterBits( ST25R3916_REG_CAP_SENSOR_CONTROL, ST25R3916_REG_CAP_SENSOR_CONTROL_cs_mcal_mask );

    ret = st25r3916ExecuteCommandAndGetResult( ST25R3916_CMD_CALIBRATE_C_SENSOR, ST25R3916_REG_CAP_SENSOR_RESULT, ST25R3916_TOUT_CALIBRATE_CAP_SENSOR, &res );

    if( ((res & ST25R3916_REG_CAP_SENSOR_RESULT_cs_cal_end) != ST25R3916_REG_CAP_SENSOR_RESULT_cs_cal_end) ||
        ((res & ST25R3916_REG_CAP_SENSOR_RESULT_cs_cal_err) == ST25R3916_REG_CAP_SENSOR_RESULT_cs_cal_err) || (ret != RFAL_ERR_NONE) )
    {
        return RFAL_ERR_IO;
    }

    if( result != NULL )
    {
        (*result) = (uint8_t)(res >> ST25R3916_REG_CAP_SENSOR_RESULT_cs_cal_shift);
    }

    return RFAL_ERR_NONE;
#endif
}

ReturnCode st25r3916SetBitrate(uint8_t txrate, uint8_t rxrate)
{
    uint8_t reg;

    st25r3916ReadRegister( ST25R3916_REG_BIT_RATE, &reg );
    if( rxrate != ST25R3916_BR_DO_NOT_SET )
    {
        if(rxrate > ST25R3916_BR_848)
        {
            return RFAL_ERR_PARAM;
        }

        reg = (uint8_t)(reg & ~ST25R3916_REG_BIT_RATE_rxrate_mask);
        reg |= rxrate << ST25R3916_REG_BIT_RATE_rxrate_shift;
    }
    if( txrate != ST25R3916_BR_DO_NOT_SET )
    {
        if(txrate > ST25R3916_BR_6780)
        {
            return RFAL_ERR_PARAM;
        }

        reg = (uint8_t)(reg & ~ST25R3916_REG_BIT_RATE_txrate_mask);
        reg |= txrate<<ST25R3916_REG_BIT_RATE_txrate_shift;

    }
    return st25r3916WriteRegister( ST25R3916_REG_BIT_RATE, reg );
}

ReturnCode st25r3916PerformCollisionAvoidance( uint8_t FieldONCmd, uint8_t pdThreshold, uint8_t caThreshold, uint8_t nTRFW )
{
    uint8_t    treMask;
    uint32_t   irqs;
    ReturnCode err;

    if( (FieldONCmd != ST25R3916_CMD_INITIAL_RF_COLLISION) && (FieldONCmd != ST25R3916_CMD_RESPONSE_RF_COLLISION_N) )
    {
        return RFAL_ERR_PARAM;
    }

    err = RFAL_ERR_INTERNAL;

    if( (pdThreshold != ST25R3916_THRESHOLD_DO_NOT_SET) || (caThreshold != ST25R3916_THRESHOLD_DO_NOT_SET) )
    {
        treMask = 0;

        if(pdThreshold != ST25R3916_THRESHOLD_DO_NOT_SET)
        {
            treMask |= ST25R3916_REG_FIELD_THRESHOLD_ACTV_trg_mask;
        }

        if(caThreshold != ST25R3916_THRESHOLD_DO_NOT_SET)
        {
            treMask |= ST25R3916_REG_FIELD_THRESHOLD_ACTV_rfe_mask;
        }

        st25r3916ChangeRegisterBits( ST25R3916_REG_FIELD_THRESHOLD_ACTV, treMask, (pdThreshold & ST25R3916_REG_FIELD_THRESHOLD_ACTV_trg_mask) | (caThreshold & ST25R3916_REG_FIELD_THRESHOLD_ACTV_rfe_mask ) );
    }

    st25r3916ChangeRegisterBits( ST25R3916_REG_AUX, ST25R3916_REG_AUX_nfc_n_mask, nTRFW );

    st25r3916GetInterrupt( (ST25R3916_IRQ_MASK_CAC | ST25R3916_IRQ_MASK_CAT | ST25R3916_IRQ_MASK_APON) );
    st25r3916EnableInterrupts( (ST25R3916_IRQ_MASK_CAC | ST25R3916_IRQ_MASK_CAT | ST25R3916_IRQ_MASK_APON) );

    st25r3916ExecuteCommand( FieldONCmd );

    irqs = st25r3916WaitForInterruptsTimed( ( ST25R3916_IRQ_MASK_CAC | ST25R3916_IRQ_MASK_APON ), ST25R3916_TOUT_CA );

    if( (ST25R3916_IRQ_MASK_CAC & irqs) != 0U )
    {
        err = RFAL_ERR_RF_COLLISION;
    }
    else if( (ST25R3916_IRQ_MASK_APON & irqs) != 0U )
    {

        irqs = st25r3916WaitForInterruptsTimed( ( ST25R3916_IRQ_MASK_CAT ), ST25R3916_TOUT_CA );

        if( (ST25R3916_IRQ_MASK_CAT & irqs) != 0U )
        {
            err = RFAL_ERR_NONE;
        }
    }
    else
    {

    }

    st25r3916GetInterrupt( (ST25R3916_IRQ_MASK_EOF | ST25R3916_IRQ_MASK_EON) );
    st25r3916DisableInterrupts( (ST25R3916_IRQ_MASK_CAC | ST25R3916_IRQ_MASK_CAT | ST25R3916_IRQ_MASK_APON) );

    return err;
}

void st25r3916SetNumTxBits( uint16_t nBits )
{
    st25r3916WriteRegister( ST25R3916_REG_NUM_TX_BYTES2, (uint8_t)((nBits >> 0) & 0xFFU) );
    st25r3916WriteRegister( ST25R3916_REG_NUM_TX_BYTES1, (uint8_t)((nBits >> 8) & 0xFFU) );
}

uint16_t st25r3916GetNumFIFOBytes( void )
{
    uint8_t  reg;
    uint16_t result;

    st25r3916ReadRegister( ST25R3916_REG_FIFO_STATUS2, &reg );
    reg    = ((reg & ST25R3916_REG_FIFO_STATUS2_fifo_b_mask) >> ST25R3916_REG_FIFO_STATUS2_fifo_b_shift);
    result = ((uint16_t)reg << 8);

    st25r3916ReadRegister( ST25R3916_REG_FIFO_STATUS1, &reg );
    result |= (((uint16_t)reg) & 0x00FFU);

    return result;
}

uint8_t st25r3916GetNumFIFOLastBits( void )
{
    uint8_t  reg;

    st25r3916ReadRegister( ST25R3916_REG_FIFO_STATUS2, &reg );

    return ((reg & ST25R3916_REG_FIFO_STATUS2_fifo_lb_mask) >> ST25R3916_REG_FIFO_STATUS2_fifo_lb_shift);
}

uint32_t st25r3916GetNoResponseTime( void )
{
    return gST25R3916NRT_64fcs;
}

ReturnCode st25r3916SetNoResponseTime( uint32_t nrt_64fcs )
{
    ReturnCode err;
    uint8_t    nrt_step;
    uint32_t   tmpNRT;

    tmpNRT = nrt_64fcs;
    err    = RFAL_ERR_NONE;

    gST25R3916NRT_64fcs = tmpNRT;
    nrt_step = ST25R3916_REG_TIMER_EMV_CONTROL_nrt_step_64fc;

    if( tmpNRT > ST25R3916_NRT_MAX )
    {
        nrt_step  = ST25R3916_REG_TIMER_EMV_CONTROL_nrt_step_4096_fc;
        tmpNRT = ((tmpNRT + 63U) / 64U);

        if( tmpNRT > ST25R3916_NRT_MAX )
        {
            tmpNRT = ST25R3916_NRT_MAX;
            err = RFAL_ERR_PARAM;
        }
        gST25R3916NRT_64fcs = (64U * tmpNRT);
    }

    st25r3916ChangeRegisterBits( ST25R3916_REG_TIMER_EMV_CONTROL, ST25R3916_REG_TIMER_EMV_CONTROL_nrt_step, nrt_step );
    st25r3916WriteRegister( ST25R3916_REG_NO_RESPONSE_TIMER1, (uint8_t)(tmpNRT >> 8U) );
    st25r3916WriteRegister( ST25R3916_REG_NO_RESPONSE_TIMER2, (uint8_t)(tmpNRT & 0xFFU) );

    return err;
}

ReturnCode st25r3916SetStartNoResponseTimer( uint32_t nrt_64fcs )
{
    ReturnCode err;

    err = st25r3916SetNoResponseTime( nrt_64fcs );
    if(err == RFAL_ERR_NONE)
    {
        st25r3916ExecuteCommand( ST25R3916_CMD_START_NO_RESPONSE_TIMER );
    }

    return err;
}

void st25r3916SetGPTime( uint16_t gpt_8fcs )
{
    st25r3916WriteRegister( ST25R3916_REG_GPT1, (uint8_t)(gpt_8fcs >> 8) );
    st25r3916WriteRegister( ST25R3916_REG_GPT2, (uint8_t)(gpt_8fcs & 0xFFU) );
}

ReturnCode st25r3916SetStartGPTimer( uint16_t gpt_8fcs, uint8_t trigger_source )
{
    st25r3916SetGPTime( gpt_8fcs );
    st25r3916ChangeRegisterBits( ST25R3916_REG_TIMER_EMV_CONTROL, ST25R3916_REG_TIMER_EMV_CONTROL_gptc_mask, trigger_source );

    if( trigger_source == ST25R3916_REG_TIMER_EMV_CONTROL_gptc_no_trigger )
    {
        st25r3916ExecuteCommand( ST25R3916_CMD_START_GP_TIMER );
    }

    return RFAL_ERR_NONE;
}

bool st25r3916CheckChipID( uint8_t *rev )
{
    uint8_t ID;

    ID = 0;
    st25r3916ReadRegister( ST25R3916_REG_IC_IDENTITY, &ID );

#if defined(ST25R3916)
    if( (ID & ST25R3916_REG_IC_IDENTITY_ic_type_mask) != ST25R3916_REG_IC_IDENTITY_ic_type_st25r3916 )
    {
        return false;
    }
#elif defined(ST25R3916B)
    if( ( (ID & ST25R3916_REG_IC_IDENTITY_ic_type_mask) != ST25R3916_REG_IC_IDENTITY_ic_type_st25r3916B ) ||
        ( (ID & ST25R3916_REG_IC_IDENTITY_ic_rev_mask) < 1U )                                                 )
    {
        return false;
    }
#endif

    if(rev != NULL)
    {
        *rev = (ID & ST25R3916_REG_IC_IDENTITY_ic_rev_mask);
    }

    return true;
}

ReturnCode st25r3916GetRegsDump( t_st25r3916Regs* regDump )
{
    uint8_t regIt;

    if(regDump == NULL)
    {
        return RFAL_ERR_PARAM;
    }

    for( regIt = ST25R3916_REG_IO_CONF1; regIt <= ST25R3916_REG_IC_IDENTITY; regIt++ )
    {
        st25r3916ReadRegister(regIt, &regDump->RsA[regIt] );
    }

    regIt = 0;

    st25r3916ReadRegister( ST25R3916_REG_EMD_SUP_CONF,      &regDump->RsB[regIt++] );
    st25r3916ReadRegister( ST25R3916_REG_SUBC_START_TIME,   &regDump->RsB[regIt++] );
    st25r3916ReadRegister( ST25R3916_REG_P2P_RX_CONF,       &regDump->RsB[regIt++] );
    st25r3916ReadRegister( ST25R3916_REG_CORR_CONF1,        &regDump->RsB[regIt++] );
    st25r3916ReadRegister( ST25R3916_REG_CORR_CONF2,        &regDump->RsB[regIt++] );
    st25r3916ReadRegister( ST25R3916_REG_SQUELCH_TIMER,     &regDump->RsB[regIt++] );
    st25r3916ReadRegister( ST25R3916_REG_FIELD_ON_GT,       &regDump->RsB[regIt++] );
    st25r3916ReadRegister( ST25R3916_REG_AUX_MOD,           &regDump->RsB[regIt++] );
    st25r3916ReadRegister( ST25R3916_REG_TX_DRIVER_TIMING,  &regDump->RsB[regIt++] );
    st25r3916ReadRegister( ST25R3916_REG_RES_AM_MOD,        &regDump->RsB[regIt++] );
    st25r3916ReadRegister( ST25R3916_REG_TX_DRIVER_STATUS,  &regDump->RsB[regIt++] );
    st25r3916ReadRegister( ST25R3916_REG_REGULATOR_RESULT,  &regDump->RsB[regIt++] );

#ifdef ST25R3916B
    st25r3916ReadRegister( ST25R3916_REG_AWS_CONF1,  &regDump->RsB[regIt++] );
    st25r3916ReadRegister( ST25R3916_REG_AWS_CONF2,  &regDump->RsB[regIt++] );
#endif

    st25r3916ReadRegister( ST25R3916_REG_OVERSHOOT_CONF1,   &regDump->RsB[regIt++] );
    st25r3916ReadRegister( ST25R3916_REG_OVERSHOOT_CONF2,   &regDump->RsB[regIt++] );
    st25r3916ReadRegister( ST25R3916_REG_UNDERSHOOT_CONF1,  &regDump->RsB[regIt++] );
    st25r3916ReadRegister( ST25R3916_REG_UNDERSHOOT_CONF2,  &regDump->RsB[regIt++] );

#ifdef ST25R3916B
    st25r3916ReadRegister( ST25R3916_REG_AWS_TIME1,   &regDump->RsB[regIt++] );
    st25r3916ReadRegister( ST25R3916_REG_AWS_TIME2,   &regDump->RsB[regIt++] );
    st25r3916ReadRegister( ST25R3916_REG_AWS_TIME3,   &regDump->RsB[regIt++] );
    st25r3916ReadRegister( ST25R3916_REG_AWS_TIME4,   &regDump->RsB[regIt++] );
    st25r3916ReadRegister( ST25R3916_REG_AWS_TIME5,   &regDump->RsB[regIt++] );
    st25r3916ReadRegister( ST25R3916_REG_AWS_RC_CAL,  &regDump->RsB[regIt++] );
#endif

    return RFAL_ERR_NONE;
}

bool st25r3916IsCmdValid( uint8_t cmd )
{
    if( (!((cmd >= ST25R3916_CMD_SET_DEFAULT)             && (cmd <= ST25R3916_CMD_RESPONSE_RF_COLLISION_N)))   &&
        (!((cmd >= ST25R3916_CMD_GOTO_SENSE)              && (cmd <= ST25R3916_CMD_GOTO_SLEEP)))                &&
        (!((cmd >= ST25R3916_CMD_MASK_RECEIVE_DATA)       && (cmd <= ST25R3916_CMD_MEASURE_AMPLITUDE)))         &&
        (!((cmd >= ST25R3916_CMD_RESET_RXGAIN)            && (cmd <= ST25R3916_CMD_ADJUST_REGULATORS)))         &&
        (!((cmd >= ST25R3916_CMD_CALIBRATE_DRIVER_TIMING) && (cmd <= ST25R3916_CMD_START_PPON2_TIMER)))         &&
    #ifdef ST25R3916B
        (cmd != ST25R3916_CMD_RC_CAL)                                                                           &&
    #endif
         (cmd != ST25R3916_CMD_SPACE_B_ACCESS)           && (cmd != ST25R3916_CMD_STOP_NRT)                      )
    {
        return false;
    }
    return true;
}

ReturnCode st25r3916StreamConfigure(const struct st25r3916StreamConfig *config)
{
    uint8_t smd;
    uint8_t mode;

    smd = 0;

    if( config->useBPSK != 0U )
    {
        mode = ST25R3916_REG_MODE_om_bpsk_stream;
        if( (config->din<2U) || (config->din>4U) )
        {
            return RFAL_ERR_PARAM;
        }
        smd |= ((4U - config->din) << ST25R3916_REG_STREAM_MODE_scf_shift);
    }
    else
    {
        mode = ST25R3916_REG_MODE_om_subcarrier_stream;
        if( (config->din<3U) || (config->din>6U) )
        {
            return RFAL_ERR_PARAM;
        }
        smd |= ((6U - config->din) << ST25R3916_REG_STREAM_MODE_scf_shift);
        if( config->report_period_length == 0U )
        {
            return RFAL_ERR_PARAM;
        }
    }

    if( (config->dout<1U) || (config->dout>7U) )
    {
        return RFAL_ERR_PARAM;
    }
    smd |= (7U - config->dout) << ST25R3916_REG_STREAM_MODE_stx_shift;

    if( config->report_period_length > 3U )
    {
        return RFAL_ERR_PARAM;
    }
    smd |= (config->report_period_length << ST25R3916_REG_STREAM_MODE_scp_shift);

    st25r3916WriteRegister(ST25R3916_REG_STREAM_MODE, smd);
    st25r3916ChangeRegisterBits(ST25R3916_REG_MODE, ST25R3916_REG_MODE_om_mask, mode);

    return RFAL_ERR_NONE;
}

ReturnCode st25r3916GetRSSI( uint16_t *amRssi, uint16_t *pmRssi )
{

    static const uint16_t st25r3916Rssi2mV[16] = { 0 ,20 ,27 ,37 ,52 ,72 ,99 ,136 ,190 ,262 ,357 ,500 ,686 ,950, 1150, 1150 };

    static const uint16_t st25r3916Gain2Percent[16] = { 100, 100, 100, 100, 100, 141, 200, 281, 398, 562, 794, 1, 1, 1, 1, 1 };

    uint8_t  rssi;
    uint8_t  gainRed;

    st25r3916ReadRegister( ST25R3916_REG_RSSI_RESULT, &rssi );
    st25r3916ReadRegister( ST25R3916_REG_GAIN_RED_STATE, &gainRed );

    if( amRssi != NULL )
    {
        *amRssi = (uint16_t) ( ( (uint32_t)st25r3916Rssi2mV[ (rssi >> ST25R3916_REG_RSSI_RESULT_rssi_am_shift) ] * (uint32_t)st25r3916Gain2Percent[ (gainRed >> ST25R3916_REG_GAIN_RED_STATE_gs_am_shift) ] ) / 100U );
    }

    if( pmRssi != NULL )
    {
        *pmRssi = (uint16_t) ( ( (uint32_t)st25r3916Rssi2mV[ (rssi & ST25R3916_REG_RSSI_RESULT_rssi_pm_mask) ] * (uint32_t)st25r3916Gain2Percent[ (gainRed & ST25R3916_REG_GAIN_RED_STATE_gs_pm_mask) ] ) / 100U );
    }

    return RFAL_ERR_NONE;
}

ReturnCode st25r3916SetAntennaMode( bool single, bool rfiox )
{
    uint8_t val;

    val  = 0U;
    val |= ((single)? ST25R3916_REG_IO_CONF1_single : 0U);
    val |= ((rfiox) ? ST25R3916_REG_IO_CONF1_rfo2   : 0U);

    st25r3916ChangeRegisterBits( ST25R3916_REG_IO_CONF1, (ST25R3916_REG_IO_CONF1_single | ST25R3916_REG_IO_CONF1_rfo2), val );
    return RFAL_ERR_NONE;
}
