#pragma once

#include <stdint.h>


class BUTTON_STRIP
{
  const int     m_i2c_address; 
  uint8_t       m_switch_values             = 0;
  uint8_t       m_step_num                  = 0;
  
  uint32_t      m_step_length_ms            = 0;
  uint64_t      m_next_step_time_stamp_ms   = 0;
  
public:

  BUTTON_STRIP( int i2c_address );
  
  bool          update( uint32_t time_ms, uint32_t& activated_segment );
  void          set_step_length( uint32_t step_length_ms );
};

