#ifndef ST25R3916_AAT_H
#define ST25R3916_AAT_H

#include "rfal_platform.h"
#include "rfal_utils.h"

struct st25r3916AatTuneParams{
    uint8_t aat_a_min;
    uint8_t aat_a_max;
    uint8_t aat_a_start;
    uint8_t aat_a_stepWidth;
    uint8_t aat_b_min;
    uint8_t aat_b_max;
    uint8_t aat_b_start;
    uint8_t aat_b_stepWidth;

    uint8_t phaTarget;
    uint8_t phaWeight;
    uint8_t ampTarget;
    uint8_t ampWeight;

    bool doDynamicSteps;
    uint8_t measureLimit;
};

struct st25r3916AatTuneResult{

    uint8_t aat_a;
    uint8_t aat_b;
    uint8_t pha;
    uint8_t amp;
    uint16_t measureCnt;
};

extern ReturnCode st25r3916AatTune(const struct st25r3916AatTuneParams *tuningParams, struct st25r3916AatTuneResult *tuningStatus);

#endif
