#include "st25r3916_led.h"
#include "st25r3916_irq.h"
#include "st25r3916_com.h"
#include "st25r3916.h"

#ifdef PLATFORM_LED_RX_PIN
    #define st25r3916ledRxOn()            platformLedOn( PLATFORM_LED_RX_PORT, PLATFORM_LED_RX_PIN );
    #define st25r3916ledRxOff()           platformLedOff( PLATFORM_LED_RX_PORT, PLATFORM_LED_RX_PIN );
#else
    #define st25r3916ledRxOn()
    #define st25r3916ledRxOff()
#endif

#ifdef PLATFORM_LED_FIELD_PIN
    #define st25r3916ledFieldOn()         platformLedOn( PLATFORM_LED_FIELD_PORT, PLATFORM_LED_FIELD_PIN );
    #define st25r3916ledFieldOff()        platformLedOff( PLATFORM_LED_FIELD_PORT, PLATFORM_LED_FIELD_PIN );
#else
    #define st25r3916ledFieldOn()
    #define st25r3916ledFieldOff()
#endif

#ifdef PLATFORM_LED_ERR_PIN
    #define st25r3916ledErrOn()         platformLedOn( PLATFORM_LED_ERR_PORT, PLATFORM_LED_ERR_PIN );
    #define st25r3916ledErrOff()        platformLedOff( PLATFORM_LED_ERR_PORT, PLATFORM_LED_ERR_PIN );
#else
    #define st25r3916ledErrOn()
    #define st25r3916ledErrOff()
#endif

void st25r3916ledInit( void )
{

    platformLedsInitialize();

    st25r3916ledRxOff();
    st25r3916ledFieldOff();
    st25r3916ledErrOff();
}

void st25r3916ledEvtIrq( uint32_t irqs )
{
    if( (irqs & (ST25R3916_IRQ_MASK_TXE | ST25R3916_IRQ_MASK_CAT) ) != 0U )
    {
        st25r3916ledFieldOn();
        st25r3916ledErrOff();
    }

    if( (irqs & (ST25R3916_IRQ_MASK_RXS | ST25R3916_IRQ_MASK_NFCT) ) != 0U )
    {
        st25r3916ledRxOn();
    }

    if( (irqs & (ST25R3916_IRQ_MASK_RXE  | ST25R3916_IRQ_MASK_NRE    | ST25R3916_IRQ_MASK_RX_REST | ST25R3916_IRQ_MASK_RXE_PTA |
                 ST25R3916_IRQ_MASK_WU_A | ST25R3916_IRQ_MASK_WU_A_X | ST25R3916_IRQ_MASK_WU_F    | ST25R3916_IRQ_MASK_RFU2)   ) != 0U )
    {
        st25r3916ledRxOff();
    }

    if( ((irqs & (ST25R3916_IRQ_MASK_CRC | ST25R3916_IRQ_MASK_PAR | ST25R3916_IRQ_MASK_ERR1 | ST25R3916_IRQ_MASK_ERR2)) != 0U) )
    {
        st25r3916ledErrOn();
    }
}

void st25r3916ledEvtWrReg( uint8_t reg, uint8_t val )
{
    if( reg == ST25R3916_REG_OP_CONTROL )
    {
        if( (ST25R3916_REG_OP_CONTROL_tx_en & val) != 0U )
        {
            st25r3916ledFieldOn();
        }
        else
        {
            st25r3916ledFieldOff();
        }
    }
}

void st25r3916ledEvtWrMultiReg( uint8_t reg, const uint8_t* vals, uint8_t len )
{
    uint8_t i;

    for(i=0; i<(len); i++)
    {
        st25r3916ledEvtWrReg( (reg+i), vals[i] );
    }
}

void st25r3916ledEvtCmd( uint8_t cmd )
{
    if( (cmd >= ST25R3916_CMD_TRANSMIT_WITH_CRC) && (cmd <= ST25R3916_CMD_RESPONSE_RF_COLLISION_N) )
    {
        st25r3916ledFieldOff();
    }

    if( cmd == ST25R3916_CMD_UNMASK_RECEIVE_DATA )
    {
        st25r3916ledRxOff();
    }

    if( cmd == ST25R3916_CMD_SET_DEFAULT )
    {
        st25r3916ledFieldOff();
        st25r3916ledRxOff();
    }
}
