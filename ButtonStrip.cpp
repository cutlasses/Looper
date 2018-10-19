#include <Wire.h>

#include "ButtonStrip.h"


constexpr int NUM_SEGMENTS(8);
 
BUTTON_STRIP::BUTTON_STRIP( int i2c_address ) :
  m_i2c_address( i2c_address )
{
  
}

bool BUTTON_STRIP::update( uint32_t time_ms, uint32_t& activated_segment )
{
  if( time_ms > m_next_step_time_stamp_ms )
  {
    if( ++m_step_num >= NUM_SEGMENTS )
    {
      m_step_num = 0;
    }
    
    m_next_step_time_stamp_ms = time_ms + m_step_length_ms;
  }

  bool step_triggered = false;
  for( int i = 0; i < NUM_SEGMENTS; ++i )
  {
    const uint8_t bit_on = 1 << i;
    if( m_switch_values & bit_on )
    {
      //Serial.print("Button press ");
      //Serial.println(i);
      m_step_num = i;
      activated_segment = i;
      step_triggered = true;
      //next_step_time_stamp = millis() + step_time_ms;
      break; // only interested in the lowest button
    }
  }

  // read switch values
  Wire.requestFrom( m_i2c_address, 1);
  m_switch_values = Wire.read();

  // write led values
  const uint8_t led_values = (1 << m_step_num);
  Wire.beginTransmission(m_i2c_address);
  Wire.write(led_values);
  Wire.endTransmission();

  return step_triggered;
}

void BUTTON_STRIP::set_step_length( uint32_t step_length_ms )
{
  m_step_length_ms = step_length_ms;
}

 int BUTTON_STRIP::num_segments() const
 {
  return NUM_SEGMENTS;
 }

