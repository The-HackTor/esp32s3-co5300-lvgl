#ifndef RFAL_CHIP_H
#define RFAL_CHIP_H

#include "rfal_platform.h"
#include "rfal_utils.h"
#include "rfal_rf.h"

ReturnCode rfalChipWriteReg( uint16_t reg, const uint8_t* values, uint8_t len );

ReturnCode rfalChipReadReg( uint16_t reg, uint8_t* values, uint8_t len );

ReturnCode rfalChipChangeRegBits( uint16_t reg, uint8_t valueMask, uint8_t value );

ReturnCode rfalChipWriteTestReg( uint16_t reg, uint8_t value );

ReturnCode rfalChipReadTestReg( uint16_t reg, uint8_t* value );

ReturnCode rfalChipChangeTestRegBits( uint16_t reg, uint8_t valueMask, uint8_t value );

ReturnCode rfalChipExecCmd( uint16_t cmd );

ReturnCode rfalChipSetRFO( uint8_t rfo );

ReturnCode rfalChipGetRFO( uint8_t* result );

ReturnCode rfalChipGetLmFieldInd( uint8_t* result );

ReturnCode rfalChipSetLMMod( uint8_t mod, uint8_t unmod );

ReturnCode rfalChipGetLMMod( uint8_t* mod, uint8_t* unmod );

ReturnCode rfalChipMeasureAmplitude( uint8_t* result );

ReturnCode rfalChipMeasurePhase( uint8_t* result );

ReturnCode rfalChipMeasureCapacitance( uint8_t* result );

ReturnCode rfalChipMeasurePowerSupply( uint8_t param, uint8_t* result );

ReturnCode rfalChipMeasureIQ( int8_t* resI, int8_t* resQ );

ReturnCode rfalChipMeasureCombinedIQ( uint8_t* result );

ReturnCode rfalChipSetAntennaMode( bool single, bool rfiox );

#endif

