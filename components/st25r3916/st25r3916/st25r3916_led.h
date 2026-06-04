#ifndef ST25R3916_LED_H
#define ST25R3916_LED_H

#include "rfal_platform.h"

void st25r3916ledInit( void );

void st25r3916ledEvtIrq( uint32_t irqs );

void st25r3916ledEvtWrReg( uint8_t reg, uint8_t val );

void st25r3916ledEvtWrMultiReg( uint8_t reg, const uint8_t* vals, uint8_t len );

void st25r3916ledEvtCmd( uint8_t cmd );

#endif

