#include "ir_protocol_color.h"
#include "ui/styles.h"
#include <string.h>

lv_color_t ir_protocol_color(const char *name)
{
    if(!name || !*name) return COLOR_DIM;
    if(strstr(name, "NEC"))        return COLOR_GREEN;
    if(strstr(name, "Samsung"))    return COLOR_ORANGE;
    if(strstr(name, "RC5"))        return COLOR_BLUE;
    if(strstr(name, "RC6"))        return COLOR_BLUE;
    if(strstr(name, "SIRC"))       return COLOR_RED;
    if(strstr(name, "Sony"))       return COLOR_RED;
    if(strstr(name, "Kaseikyo"))   return COLOR_YELLOW;
    if(strstr(name, "Panasonic"))  return COLOR_YELLOW;
    if(strstr(name, "Pioneer"))    return COLOR_MAGENTA;
    if(strstr(name, "RCA"))        return COLOR_CYAN;
    if(strstr(name, "Daikin"))     return COLOR_CYAN;
    if(strstr(name, "Mitsubishi")) return COLOR_CYAN;
    if(strstr(name, "Hitachi"))    return COLOR_CYAN;
    if(strstr(name, "Carrier"))    return COLOR_CYAN;
    if(strstr(name, "Bose"))       return COLOR_MAGENTA;
    if(strstr(name, "Lutron"))     return COLOR_BLUE;
    if(strstr(name, "Magi"))       return COLOR_MAGENTA;
    if(strstr(name, "AC"))         return COLOR_CYAN;
    return COLOR_DIM;
}
