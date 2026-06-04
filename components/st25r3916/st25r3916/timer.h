#ifndef TIMER_H
#define TIMER_H

#include "rfal_platform.h"

#define timerIsRunning(t)            (!timerIsExpired(t))

uint32_t timerCalculateTimer( uint16_t time );

bool timerIsExpired( uint32_t timer );

void timerDelay( uint16_t time );

void timerStopwatchStart( void );

uint32_t timerStopwatchMeasure( void );

#endif
