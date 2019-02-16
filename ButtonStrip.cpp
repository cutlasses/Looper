#include <Wire.h>

#include "ButtonStrip.h"

 
BUTTON_STRIP::BUTTON_STRIP( int i2c_address ) :
  m_i2c_address( i2c_address )
{
}

void BUTTON_STRIP::send_led_values(uint8_t led_values)
{
  Wire.beginTransmission(m_i2c_address);
  Wire.write(led_values);
  Wire.endTransmission();  
}

void BUTTON_STRIP::update_free_play()
{
  
}

bool BUTTON_STRIP::update( uint32_t time_ms, uint32_t& activated_segment )
{
  bool step_advanced = false;
  if( time_ms > m_next_step_time_stamp_ms )
  {
    if( ++m_step_num >= NUM_SEGMENTS )
    {
      m_step_num = 0;
    }

    step_advanced = true;
    m_next_step_time_stamp_ms = time_ms + m_step_length_ms;
  }

  bool step_triggered = false;
  
  if( !m_buttons_locked )
  {
    for( int i = 0; i < NUM_SEGMENTS; ++i )
    {
      const uint8_t bit_on        = 1 << i;
      DEBOUNCE_DETAILS& debounce  = m_debounce_details[i];
      const bool button_down      = m_switch_values & bit_on;
  
      if( button_down != debounce.m_button_down )
      {
        // state change - record time stamp
        debounce.m_time_stamp     = time_ms;
        debounce.m_button_down    = button_down;
        debounce.m_registered     = false;
      }
  
      if( debounce.m_button_down &&
          !debounce.m_registered &&
          ( (time_ms - debounce.m_time_stamp) > BUTTON_DEBOUNCE_MS ) )
      {
          // button pressed 
          debounce.m_registered     = true;
          
          m_step_num                = i;
          activated_segment         = i;
          step_triggered            = true;
          step_advanced             = true;
          m_next_step_time_stamp_ms = time_ms + m_step_length_ms;
  
          break; // only interested in the lowest button
      }
    }
  }
  
  // read switch values
  if( Wire.requestFrom( m_i2c_address, 1) )
  {
    m_switch_values = Wire.read();
  }

  // write led values
  if( step_advanced )
  {    
    const uint8_t led_values = m_running ? (1 << m_step_num) : 0;
    send_led_values( led_values );
  }

  return step_triggered;
}

void BUTTON_STRIP::start_sequence( uint32_t sequence_length_ms, uint32_t current_time_ms )
{
  m_step_length_ms          = sequence_length_ms / NUM_SEGMENTS;
    
  m_running                 = true;
  m_step_num                = NUM_SEGMENTS; // next update will move to step 0
  m_next_step_time_stamp_ms = current_time_ms;
}

void BUTTON_STRIP::stop_sequence()
{
  m_running = false;
}


void BUTTON_STRIP::lock_buttons( bool lock )
{
  m_buttons_locked = lock;
}

int BUTTON_STRIP::num_segments() const
{
  return NUM_SEGMENTS;
}

