#pragma once

#include "Interface.h"
#include "Util.h"

class LOOPER_INTERFACE
{
public:

  enum class MODE
  {
    LOOP_PLAY,
    LOOP_RECORD,
    NUM_MODES,
  };

private:
  
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

    int                       m_current_play_back_sample;
    int                       m_num_samples;
    MODE                      m_mode;
    bool                      m_mode_pending;

  public:

    LOOPER_INTERFACE();
    void                      setup( int num_samples );

    bool                      update( ADC& adc, uint32_t time_in_ms );
    void                      set_recording( bool recording, uint32_t time_in_ms );
    void                      set_mode_pending( bool pending, uint32_t time_in_ms );

    MODE                      mode() const;
    const BUTTON&             record_button() const;
    bool                      sample_to_play( int& sample_index );

    float                     gain() const;
    float                     saturation() const;       // not on a dial
    float                     play_back_speed() const;
    float                     delay_time() const;
    float                     delay_feedback() const;
    float                     delay_mix() const;
    float                     looper_mix() const;
};
