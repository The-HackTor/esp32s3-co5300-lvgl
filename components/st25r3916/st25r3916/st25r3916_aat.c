#include <rfal_platform.h>
#include "st25r3916_aat.h"
#include "rfal_utils.h"
#include "st25r3916.h"
#include "st25r3916_com.h"
#include "rfal_chip.h"

#define ST25R3916_AAT_CAP_DELAY_MAX           10

#define st25r3916AatLog(...)

static ReturnCode aatHillClimb(const struct st25r3916AatTuneParams *tuningParams, struct st25r3916AatTuneResult *tuningStatus);
static int32_t aatGreedyDescent(uint32_t *f_min, const struct st25r3916AatTuneParams *tuningParams, struct st25r3916AatTuneResult *tuningStatus, int32_t previousDir);
static int32_t aatSteepestDescent(uint32_t *f_min, const struct st25r3916AatTuneParams *tuningParams, struct st25r3916AatTuneResult *tuningStatus, int32_t previousDir, int32_t previousDir2);

static ReturnCode aatMeasure(uint8_t serCap, uint8_t parCap, uint8_t *amplitude, uint8_t *phase, uint16_t *measureCnt);
static uint32_t aatCalcF(const struct st25r3916AatTuneParams *tuningParams, uint8_t amplitude, uint8_t phase);
static ReturnCode aatStepDacVals(const struct st25r3916AatTuneParams *tuningParams,uint8_t *a, uint8_t *b, int32_t dir);

ReturnCode st25r3916AatTune(const struct st25r3916AatTuneParams *tuningParams, struct st25r3916AatTuneResult *tuningStatus)
{
    ReturnCode err;
    const struct st25r3916AatTuneParams *tp = tuningParams;
    struct st25r3916AatTuneResult *ts = tuningStatus;
    struct st25r3916AatTuneParams defaultTuningParams =
    {
        .aat_a_min=0,
        .aat_a_max=255,
        .aat_a_start=127,
        .aat_a_stepWidth=32,
        .aat_b_min=0,
        .aat_b_max=255,
        .aat_b_start=127,
        .aat_b_stepWidth=32,

        .phaTarget=128,
        .phaWeight=2,
        .ampTarget=196,
        .ampWeight=1,

        .doDynamicSteps=true,
        .measureLimit=50,
    };
    struct st25r3916AatTuneResult defaultTuneResult;

    if ((NULL != tp) && (
          (tp->aat_a_min > tp->aat_a_max   )
       || (tp->aat_a_start < tp->aat_a_min )
       || (tp->aat_a_start > tp->aat_a_max )
       || (tp->aat_b_min > tp->aat_b_max   )
       || (tp->aat_b_start < tp->aat_b_min )
       || (tp->aat_b_start > tp->aat_b_max )
       ))
    {
        return RFAL_ERR_PARAM;
    }

    if (NULL == tp)
    {
        st25r3916ReadRegister(ST25R3916_REG_ANT_TUNE_A, &defaultTuningParams.aat_a_start);
        st25r3916ReadRegister(ST25R3916_REG_ANT_TUNE_B, &defaultTuningParams.aat_b_start);
        tp = &defaultTuningParams;
    }

    if (NULL == ts){ts = &defaultTuneResult;}

    ts->measureCnt = 0;

    err = aatHillClimb(tp, ts);

    return err;
}

static ReturnCode aatHillClimb(const struct st25r3916AatTuneParams *tuningParams, struct st25r3916AatTuneResult *tuningStatus)
{
    ReturnCode  err = RFAL_ERR_NONE;
    uint32_t f_min;
    int32_t direction, gdirection;
    uint8_t amp,phs;
    struct st25r3916AatTuneParams tp = *tuningParams;

    tuningStatus->aat_a = tuningParams->aat_a_start;
    tuningStatus->aat_b = tuningParams->aat_b_start;

    aatMeasure(tuningStatus->aat_a,tuningStatus->aat_b,&amp,&phs,&tuningStatus->measureCnt);
    f_min = aatCalcF(&tp, amp, phs);
    direction = 0;

    st25r3916AatLog("%d %d: %d***\n",tuningStatus->aat_a,tuningStatus->aat_b,f_min);

    do {
        direction = 0;
        do {

            direction = aatSteepestDescent(&f_min, &tp, tuningStatus, direction, -direction);
            if (tuningStatus->measureCnt > tp.measureLimit)
            {
                err = RFAL_ERR_OVERRUN;
                break;
            }
            do
            {
                gdirection = aatGreedyDescent(&f_min, &tp, tuningStatus, direction);
                if (tuningStatus->measureCnt > tp.measureLimit) {
                    err = RFAL_ERR_OVERRUN;
                    break;
                }
            } while (0 != gdirection);
        } while (0 != direction);
        tp.aat_a_stepWidth /= 2U;
        tp.aat_b_stepWidth /= 2U;
    } while ((tp.doDynamicSteps) && ((tp.aat_a_stepWidth>0U) || (tp.aat_b_stepWidth>0U)));

    return err;
}

static int32_t aatSteepestDescent(uint32_t *f_min, const struct st25r3916AatTuneParams *tuningParams, struct st25r3916AatTuneResult *tuningStatus, int32_t previousDir, int32_t previousDir2)
{
    int32_t i;
    uint8_t amp,phs;
    uint32_t f;
    int32_t bestdir = 0;

    for (i = -2; i <= 2; i++)
    {
        uint8_t a = tuningStatus->aat_a , b = tuningStatus->aat_b;

        if ((0==i) || (i==-previousDir) || (i==-previousDir2))
        {
            continue;
        }
        if (0U!=aatStepDacVals(tuningParams, &a, &b, i))
        {
            continue;
        }

        aatMeasure(a,b,&amp,&phs,&tuningStatus->measureCnt);
        f = aatCalcF(tuningParams, amp, phs);
        st25r3916AatLog("%d : %d %d: %d",i,a, b, f);
        if (f < *f_min)
        {
            st25r3916AatLog("*");
            *f_min = f;
            bestdir = i;
        }
        st25r3916AatLog("\n");
    }
    if (0!=bestdir)
    {
        aatStepDacVals(tuningParams, &tuningStatus->aat_a, &tuningStatus->aat_b, bestdir);
    }
    return bestdir;
}

static int32_t aatGreedyDescent(uint32_t *f_min, const struct st25r3916AatTuneParams *tuningParams, struct st25r3916AatTuneResult *tuningStatus, int32_t previousDir)
{
    uint8_t amp,phs;
    uint32_t f;
    uint8_t a = tuningStatus->aat_a , b = tuningStatus->aat_b;

    if (0U != aatStepDacVals(tuningParams, &a, &b, previousDir))
    {
        return 0;
    }

    aatMeasure(a,b,&amp,&phs,&tuningStatus->measureCnt);
    f = aatCalcF(tuningParams, amp, phs);
    st25r3916AatLog("g : %d %d: %d",a, b, f);
    if (f < *f_min)
    {
        st25r3916AatLog("*\n");
        tuningStatus->aat_a = a;
        tuningStatus->aat_b = b;
        *f_min = f;
        return previousDir;
    }

    st25r3916AatLog("\n");
    return 0;
}

static uint32_t aatCalcF(const struct st25r3916AatTuneParams *tuningParams, uint8_t amplitude, uint8_t phase)
{

    uint8_t ampTarget = tuningParams->ampTarget;
    uint8_t phaTarget = tuningParams->phaTarget;

    uint32_t ampWeight = tuningParams->ampWeight;
    uint32_t phaWeight = tuningParams->phaWeight;

    uint8_t ad = ((amplitude > ampTarget)  ? (amplitude - ampTarget) : (ampTarget - amplitude));
    uint8_t pd = ((phase > phaTarget)      ? (phase - phaTarget)     : (phaTarget - phase));

    uint32_t ampDelta = (uint32_t)ad;
    uint32_t phaDelta = (uint32_t)pd;

    return ((ampWeight * ampDelta) + (phaWeight * phaDelta));
}

static ReturnCode aatStepDacVals(const struct st25r3916AatTuneParams *tuningParams,uint8_t *a, uint8_t *b, int32_t dir)
{
    int16_t aat_a = (int16_t)*a, aat_b = (int16_t)*b;

    switch (abs(dir))
    {
        case 1:
            aat_a = (dir<0)?(aat_a - (int16_t)tuningParams->aat_a_stepWidth):(aat_a + (int16_t)tuningParams->aat_a_stepWidth);
            if(aat_a < (int16_t)tuningParams->aat_a_min){ aat_a = (int16_t)tuningParams->aat_a_min; }
            if(aat_a > (int16_t)tuningParams->aat_a_max){ aat_a = (int16_t)tuningParams->aat_a_max; }
            if ((int16_t)*a == aat_a) {return RFAL_ERR_PARAM;}
            break;
        case 2:
            aat_b = (dir<0)?(aat_b - (int16_t)tuningParams->aat_b_stepWidth):(aat_b + (int16_t)tuningParams->aat_b_stepWidth);
            if(aat_b < (int16_t)tuningParams->aat_b_min){ aat_b = (int16_t)tuningParams->aat_b_min; }
            if(aat_b > (int16_t)tuningParams->aat_b_max){ aat_b = (int16_t)tuningParams->aat_b_max; }
            if ((int16_t)*b == aat_b) {return RFAL_ERR_PARAM;}
            break;
        default:
            return RFAL_ERR_REQUEST;
    }

    *a = (uint8_t)aat_a;
    *b = (uint8_t)aat_b;

    return RFAL_ERR_NONE;

}

static ReturnCode aatMeasure(uint8_t serCap, uint8_t parCap, uint8_t *amplitude, uint8_t *phase, uint16_t *measureCnt)
{
    ReturnCode err;

    *amplitude = 0;
    *phase     = 0;

    st25r3916WriteRegister(ST25R3916_REG_ANT_TUNE_A, serCap);
    st25r3916WriteRegister(ST25R3916_REG_ANT_TUNE_B, parCap);

    platformDelay( ST25R3916_AAT_CAP_DELAY_MAX );

    err = rfalChipMeasureAmplitude(amplitude);
    if (RFAL_ERR_NONE == err)
    {
        err = rfalChipMeasurePhase(phase);
    }

    if( measureCnt != NULL )
    {
        (*measureCnt)++;
    }
    return err;
}
