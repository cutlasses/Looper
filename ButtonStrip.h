#pragma once

#include <stdint.h>


class BUTTON_STRIP
{
  static const constexpr int NUM_SEGMENTS   = 8;
  static const constexpr int32_t BUTTON_DEBOUNCE_MS = 5;
  static const constexpr int64_t LED_I2C_UPDATE_TIME_MS = 30;

  struct DEBOUNCE_DETAILS
  {
    bool            m_button_down               = false;
    bool            m_registered                = false;
    uint64_t        m_time_stamp                = 0;
  };
  
  const int         m_i2c_address; 
  uint8_t           m_switch_values             = 0;
  uint8_t           m_step_num                  = 0;
  
  uint32_t          m_step_length_ms            = 0;
  uint64_t          m_next_step_time_stamp_ms   = 0;

  bool              m_running                   = false;

  DEBOUNCE_DETAILS  m_debounce_details[NUM_SEGMENTS];
  
public:

  BUTTON_STRIP( int i2c_address );
  
  bool              update( uint32_t time_ms, uint32_t& activated_segment );
  void              start_sequence( uint32_t sequence_length_ms, uint32_t current_time_ms );
  void              stop_sequence();

  int               num_segments() const;
};

