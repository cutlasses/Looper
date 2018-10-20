#pragma once

#include "Interface.h"
#include "Util.h"

class LOOPER_INTERFACE
{
    static constexpr int      MODE_BUTTON_PIN               = 2;
    static constexpr int      RECORD_BUTTON_PIN             = 1;
    static constexpr int      LED_1_PIN                     = 29;
    static constexpr int      LED_2_PIN                     = 11;
    static constexpr int      LED_3_PIN                     = 7;

    static constexpr int      NUM_DIALS                     = 6;
    static constexpr int      NUM_LEDS                      = 3;

    DIAL                      m_dials[NUM_DIALS];

    BUTTON                    m_mode_button;
    BUTTON                    m_record_button;

    LED                       m_leds[NUM_LEDS];

  public:

    LOOPER_INTERFACE();
    void                      setup();

    void                      update( ADC& adc, uint32_t time_in_ms );
};
