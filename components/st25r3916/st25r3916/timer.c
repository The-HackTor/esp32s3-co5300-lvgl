#include "timer.h"

static uint32_t timerStopwatchTick;

uint32_t timerCalculateTimer( uint16_t time )
{
  return (platformGetSysTick() + time);
}

bool timerIsExpired( uint32_t timer )
{
  uint32_t uDiff;
  int32_t sDiff;

  uDiff = (timer - platformGetSysTick());
  sDiff = uDiff;

  if( sDiff < 0 )
  {
    return true;
  }

  return false;
}

void timerDelay( uint16_t tOut )
{
  uint32_t t;

  t = timerCalculateTimer( tOut );
  while( timerIsRunning(t) );
}

void timerStopwatchStart( void )
{
  timerStopwatchTick = platformGetSysTick();
}

uint32_t timerStopwatchMeasure( void )
{
  return (uint32_t)(platformGetSysTick() - timerStopwatchTick);
}

