#pragma once

#include <stdint.h>


class BUTTON_STRIP
{
  static const constexpr int NUM_SEGMENTS   = 8;
  static const constexpr int32_t BUTTON_DEBOUNCE_MS = 5;
  static const constexpr int64_t LED_I2C_UPDATE_TIME_MS = 30;
  
  const int     m_i2c_address; 
  uint8_t       m_switch_values             = 0;
  uint8_t       m_step_num                  = 0;
  
  uint32_t      m_step_length_ms            = 0;
  uint64_t      m_next_step_time_stamp_ms   = 0;

  uint64_t      m_next_i2c_time_stamp_ms    = 0;

  bool          m_running                   = false;

  uint64_t      m_switch_time_stamps[NUM_SEGMENTS];
  
public:

  BUTTON_STRIP( int i2c_address );
  
  bool          update( uint32_t time_ms, uint32_t& activated_segment );
  void          set_sequence_length( uint32_t sequence_length_ms );
  void          stop_sequence();

  int           num_segments() const;
};

