#include <rfal_platform.h>
#include "st25r3916.h"
#include "st25r3916_com.h"
#include "st25r3916_led.h"
#include "rfal_utils.h"

#define ST25R3916_OPTIMIZE              true
#define ST25R3916_I2C_ADDR              (0xA0U >> 1)
#define ST25R3916_REG_LEN               1U
#define ST25R3916_MOSI_IDLE             (0x00)

#define ST25R3916_WRITE_MODE            (0U << 6)
#define ST25R3916_READ_MODE             (1U << 6)
#define ST25R3916_CMD_MODE              (3U << 6)
#define ST25R3916_FIFO_LOAD             (0x80U)
#define ST25R3916_FIFO_READ             (0x9FU)
#define ST25R3916_PT_A_CONFIG_LOAD      (0xA0U)
#define ST25R3916_PT_F_CONFIG_LOAD      (0xA8U)
#define ST25R3916_PT_TSN_DATA_LOAD      (0xACU)
#define ST25R3916_PT_MEM_READ           (0xBFU)

#define ST25R3916_CMD_LEN               (1U)
#define ST25R3916_BUF_LEN               (ST25R3916_CMD_LEN+ST25R3916_FIFO_DEPTH)

#ifdef RFAL_USE_I2C
#define st25r3916I2CStart()               platformI2CStart()
#define st25r3916I2CStop()                platformI2CStop()
#define st25r3916I2CRepeatStart()         platformI2CRepeatStart()
#define st25r3916I2CSlaveAddrWR( sA )     platformI2CSlaveAddrWR( sA )
#define st25r3916I2CSlaveAddrRD( sA )     platformI2CSlaveAddrRD( sA )
#endif

#if defined(ST25R_COM_SINGLETXRX) && !defined(RFAL_USE_I2C)
static uint8_t  comBuf[ST25R3916_BUF_LEN];
static uint16_t comBufIt;
#endif

static void st25r3916comStart( void );

static void st25r3916comStop( void );

#ifdef RFAL_USE_I2C
static void st25r3916comRepeatStart( void );
#else
#define st25r3916comRepeatStart()
#endif

static void st25r3916comTx( const uint8_t* txBuf, uint16_t txLen, bool last, bool txOnly );

static void st25r3916comRx( uint8_t* rxBuf, uint16_t rxLen );

static void st25r3916comTxByte( uint8_t txByte, bool last, bool txOnly );

static void st25r3916comStart( void )
{

    platformProtectST25RComm();

#ifdef RFAL_USE_I2C

    st25r3916I2CStart();
    st25r3916I2CSlaveAddrWR( ST25R3916_I2C_ADDR );
#else

    platformSpiSelect();

    #if defined(ST25R_COM_SINGLETXRX)
        comBufIt = 0;
    #endif

#endif

}

static void st25r3916comStop( void )
{
#ifdef RFAL_USE_I2C

    st25r3916I2CStop();
#else

    platformSpiDeselect();
#endif

    platformUnprotectST25RComm();
}

#ifdef RFAL_USE_I2C
static void st25r3916comRepeatStart( void )
{
    st25r3916I2CRepeatStart();
    st25r3916I2CSlaveAddrRD( ST25R3916_I2C_ADDR );
}
#endif

static void st25r3916comTx( const uint8_t* txBuf, uint16_t txLen, bool last, bool txOnly )
{
    uint8_t *rxBuf=0;
	RFAL_NO_WARNING(last);
    RFAL_NO_WARNING(txOnly);

    if( txLen > 0U )
    {
#ifdef RFAL_USE_I2C
        platformI2CTx( txBuf, txLen, last, txOnly );
#else

        #ifdef ST25R_COM_SINGLETXRX

            RFAL_MEMCPY( &comBuf[comBufIt], txBuf, RFAL_MIN( txLen, (uint16_t)(ST25R3916_BUF_LEN - comBufIt) ) );
            comBufIt += RFAL_MIN( txLen, (ST25R3916_BUF_LEN - comBufIt) );

            if( last && txOnly )
            {
                platformSpiTxRx( comBuf, NULL, comBufIt );
            }

        #else
            platformSpiTxRx( txBuf, rxBuf, txLen );
        #endif

#endif
    }
}

static void st25r3916comRx( uint8_t* rxBuf, uint16_t rxLen )
{
#ifndef ST25R_COM_SINGLETXRX
    uint8_t  dummyBuf;
    uint16_t rxIt;
#endif

    if( rxLen > 0U )
    {
#ifdef RFAL_USE_I2C
        platformI2CRx( rxBuf, rxLen );
#else

    #ifdef ST25R_COM_SINGLETXRX
        RFAL_MEMSET( &comBuf[comBufIt], ST25R3916_MOSI_IDLE, RFAL_MIN( rxLen, (uint16_t)(ST25R3916_BUF_LEN - comBufIt) ) );
        platformSpiTxRx( comBuf, comBuf, RFAL_MIN( (comBufIt + rxLen), ST25R3916_BUF_LEN ) );
        if( rxBuf != NULL )
        {
            RFAL_MEMCPY( rxBuf, &comBuf[comBufIt], RFAL_MIN( rxLen, (uint16_t)(ST25R3916_BUF_LEN - comBufIt) ) );
        }
    #else

        if( rxBuf == NULL )
        {
            for( rxIt = 0; (rxIt < rxLen); rxIt++ )
            {
                dummyBuf = ST25R3916_MOSI_IDLE;
                platformSpiTxRx( &dummyBuf, &dummyBuf, 1U );
            }
        }
        else
        {
            RFAL_MEMSET( rxBuf, ST25R3916_MOSI_IDLE, rxLen );
            platformSpiTxRx( rxBuf, rxBuf, rxLen );
        }

    #endif
#endif
    }
}

static void st25r3916comTxByte( uint8_t txByte, bool last, bool txOnly )
{
    uint8_t val = txByte;
    st25r3916comTx( &val, ST25R3916_REG_LEN, last, txOnly );
}

ReturnCode st25r3916ReadRegister( uint8_t reg, uint8_t* val )
{
    return st25r3916ReadMultipleRegisters( reg, val, ST25R3916_REG_LEN );
}

ReturnCode st25r3916ReadMultipleRegisters( uint8_t reg, uint8_t* values, uint8_t length )
{
    if( length > 0U )
    {
        st25r3916comStart();

        if( (reg & ST25R3916_SPACE_B) != 0U )
        {
            st25r3916comTxByte( ST25R3916_CMD_SPACE_B_ACCESS, false, false );
        }

        st25r3916comTxByte( ((reg & ~ST25R3916_SPACE_B) | ST25R3916_READ_MODE), true, false );
        st25r3916comRepeatStart();
        st25r3916comRx( values, length );
        st25r3916comStop();
    }

    return RFAL_ERR_NONE;
}

ReturnCode st25r3916WriteRegister( uint8_t reg, uint8_t val )
{
    uint8_t value = val;
    return st25r3916WriteMultipleRegisters( reg, &value, ST25R3916_REG_LEN );
}

ReturnCode st25r3916WriteMultipleRegisters( uint8_t reg, const uint8_t* values, uint8_t length )
{
    if( length > 0U )
    {
        st25r3916comStart();

        if( (reg & ST25R3916_SPACE_B) != 0U )
        {
            st25r3916comTxByte( ST25R3916_CMD_SPACE_B_ACCESS, false, true );
        }

        st25r3916comTxByte( ((reg & ~ST25R3916_SPACE_B) | ST25R3916_WRITE_MODE), false, true );
        st25r3916comTx( values, length, true, true );
        st25r3916comStop();

        st25r3916ledEvtWrMultiReg( reg, values, length);
    }

    return RFAL_ERR_NONE;
}

ReturnCode st25r3916WriteFifo( const uint8_t* values, uint16_t length )
{
    if( length > ST25R3916_FIFO_DEPTH )
    {
        return RFAL_ERR_PARAM;
    }

    if( length > 0U )
    {
        st25r3916comStart();
        st25r3916comTxByte( ST25R3916_FIFO_LOAD, false, true );
        st25r3916comTx( values, length, true, true );
        st25r3916comStop();
    }

    return RFAL_ERR_NONE;
}

ReturnCode st25r3916ReadFifo( uint8_t* buf, uint16_t length )
{
    if( length > 0U )
    {
        st25r3916comStart();
        st25r3916comTxByte( ST25R3916_FIFO_READ, true, false );

        st25r3916comRepeatStart();
        st25r3916comRx( buf, length );
        st25r3916comStop();
    }

    return RFAL_ERR_NONE;
}

ReturnCode st25r3916WritePTMem( const uint8_t* values, uint16_t length )
{
    if( length > ST25R3916_PTM_LEN )
    {
        return RFAL_ERR_PARAM;
    }

    if( length > 0U )
    {
        st25r3916comStart();
        st25r3916comTxByte( ST25R3916_PT_A_CONFIG_LOAD, false, true );
        st25r3916comTx( values, length, true, true );
        st25r3916comStop();
    }

    return RFAL_ERR_NONE;
}

ReturnCode st25r3916ReadPTMem( uint8_t* values, uint16_t length )
{
    uint8_t tmp[ST25R3916_REG_LEN + ST25R3916_PTM_LEN];

    if( length > 0U )
    {
        if( length > ST25R3916_PTM_LEN )
        {
            return RFAL_ERR_PARAM;
        }

        st25r3916comStart();
        st25r3916comTxByte( ST25R3916_PT_MEM_READ, true, false );

        st25r3916comRepeatStart();
        st25r3916comRx( tmp, (ST25R3916_REG_LEN + length) );
        st25r3916comStop();

        RFAL_MEMCPY( values, (tmp+ST25R3916_REG_LEN), length );
    }

    return RFAL_ERR_NONE;
}

ReturnCode st25r3916WritePTMemF( const uint8_t* values, uint16_t length )
{
    if( length > (ST25R3916_PTM_F_LEN + ST25R3916_PTM_TSN_LEN) )
    {
        return RFAL_ERR_PARAM;
    }

    if( length > 0U )
    {
        st25r3916comStart();
        st25r3916comTxByte( ST25R3916_PT_F_CONFIG_LOAD, false, true );
        st25r3916comTx( values, length, true, true );
        st25r3916comStop();
    }

    return RFAL_ERR_NONE;
}

ReturnCode st25r3916WritePTMemTSN( const uint8_t* values, uint16_t length )
{
    if( length > ST25R3916_PTM_TSN_LEN )
    {
        return RFAL_ERR_PARAM;
    }

    if(length > 0U)
    {
        st25r3916comStart();
        st25r3916comTxByte( ST25R3916_PT_TSN_DATA_LOAD, false, true );
        st25r3916comTx( values, length, true, true );
        st25r3916comStop();
    }

    return RFAL_ERR_NONE;
}

ReturnCode st25r3916ExecuteCommand( uint8_t cmd )
{
    st25r3916comStart();
    st25r3916comTxByte( (cmd | ST25R3916_CMD_MODE ), true, true );
    st25r3916comStop();

    st25r3916ledEvtCmd(cmd);

    return RFAL_ERR_NONE;
}

ReturnCode st25r3916ReadTestRegister( uint8_t reg, uint8_t* val )
{
    st25r3916comStart();
    st25r3916comTxByte( ST25R3916_CMD_TEST_ACCESS, false, false );
    st25r3916comTxByte( (reg | ST25R3916_READ_MODE), true, false );
    st25r3916comRepeatStart();
    st25r3916comRx( val, ST25R3916_REG_LEN );
    st25r3916comStop();

    return RFAL_ERR_NONE;
}

ReturnCode st25r3916WriteTestRegister( uint8_t reg, uint8_t val )
{
    uint8_t value = val;

    st25r3916comStart();
    st25r3916comTxByte( ST25R3916_CMD_TEST_ACCESS, false, true );
    st25r3916comTxByte( (reg | ST25R3916_WRITE_MODE), false, true );
    st25r3916comTx( &value, ST25R3916_REG_LEN, true, true );
    st25r3916comStop();

    return RFAL_ERR_NONE;
}

ReturnCode st25r3916ClrRegisterBits( uint8_t reg, uint8_t clr_mask )
{
    ReturnCode ret;
    uint8_t    rdVal;

    RFAL_EXIT_ON_ERR( ret, st25r3916ReadRegister(reg, &rdVal) );

    if( ST25R3916_OPTIMIZE && (rdVal == (uint8_t)(rdVal & ~clr_mask)) )
    {
        return RFAL_ERR_NONE;
    }

    return st25r3916WriteRegister(reg, (uint8_t)(rdVal & ~clr_mask) );
}

ReturnCode st25r3916SetRegisterBits( uint8_t reg, uint8_t set_mask )
{
    ReturnCode ret;
    uint8_t    rdVal;

    RFAL_EXIT_ON_ERR( ret, st25r3916ReadRegister(reg, &rdVal) );

    if( ST25R3916_OPTIMIZE && (rdVal == (rdVal | set_mask)) )
    {
        return RFAL_ERR_NONE;
    }

    return st25r3916WriteRegister(reg, (rdVal | set_mask) );
}

ReturnCode st25r3916ChangeRegisterBits( uint8_t reg, uint8_t valueMask, uint8_t value )
{
    return st25r3916ModifyRegister(reg, valueMask, (valueMask & value) );
}

ReturnCode st25r3916ModifyRegister( uint8_t reg, uint8_t clr_mask, uint8_t set_mask )
{
    ReturnCode ret;
    uint8_t    rdVal;
    uint8_t    wrVal;

    RFAL_EXIT_ON_ERR( ret, st25r3916ReadRegister(reg, &rdVal) );

    wrVal  = (uint8_t)(rdVal & ~clr_mask);
    wrVal |= set_mask;

    if( ST25R3916_OPTIMIZE && (rdVal == wrVal) )
    {
        return RFAL_ERR_NONE;
    }

    return st25r3916WriteRegister(reg, wrVal );
}

ReturnCode st25r3916ChangeTestRegisterBits( uint8_t reg, uint8_t valueMask, uint8_t value )
{
    ReturnCode ret;
    uint8_t    rdVal;
    uint8_t    wrVal;

    RFAL_EXIT_ON_ERR( ret, st25r3916ReadTestRegister(reg, &rdVal) );

    wrVal  = (uint8_t)(rdVal & ~valueMask);
    wrVal |= (uint8_t)(value & valueMask);

    if( ST25R3916_OPTIMIZE && (rdVal == wrVal) )
    {
        return RFAL_ERR_NONE;
    }

    return st25r3916WriteTestRegister(reg, wrVal );
}

bool st25r3916CheckReg( uint8_t reg, uint8_t mask, uint8_t val )
{
    uint8_t regVal;

    regVal = 0;
    st25r3916ReadRegister( reg, &regVal );

    return ( (regVal & mask) == val );
}

bool st25r3916IsRegValid( uint8_t reg )
{
    if( !(( (int16_t)reg >= (int16_t)ST25R3916_REG_IO_CONF1) && (reg <= (ST25R3916_SPACE_B | ST25R3916_REG_IC_IDENTITY)) ))
    {
        return false;
    }
    return true;
}

