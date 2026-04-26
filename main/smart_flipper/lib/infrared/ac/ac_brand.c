#include "ac_brand.h"

const char *ac_mode_label(AcMode m)
{
    switch(m) {
    case AC_MODE_AUTO: return "Auto";
    case AC_MODE_COOL: return "Cool";
    case AC_MODE_DRY:  return "Dry";
    case AC_MODE_FAN:  return "Fan";
    case AC_MODE_HEAT: return "Heat";
    default:           return "?";
    }
}

const char *ac_fan_label(AcFan f)
{
    switch(f) {
    case AC_FAN_AUTO: return "Auto";
    case AC_FAN_LOW:  return "Low";
    case AC_FAN_MED:  return "Med";
    case AC_FAN_HIGH: return "High";
    default:          return "?";
    }
}
