#include <Wire.h>

#include "ButtonStrip.h"
#include "Util.h"

 
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

bool BUTTON_STRIP::update_free_play( uint32_t time_ms, uint32_t& activated_segment )
{
  bool step_triggered = false;
  bool update_leds = false;
  if( time_ms > m_next_step_time_stamp_ms )
  {
    if( ++m_step_num >= NUM_SEGMENTS )
    {
      m_step_num = 0;
    }

    update_leds = true;
    m_next_step_time_stamp_ms = time_ms + m_step_length_ms;
  }
  
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
          update_leds               = true;
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
  if( update_leds )
  {    
    const uint8_t led_values = m_running ? (1 << m_step_num) : 0;
    send_led_values( led_values );
  }

  return step_triggered;  
}

void BUTTON_STRIP::record_sequence_event( uint32_t time_ms, int32_t activated_segment )
{
  // TODO end the sequence (with final event) when it gets full
  ASSERT_MSG( m_current_seq_num_events < MAX_SEQUENCE_EVENTS, "Max events exceeded" );
  const uint32_t event_time = time_ms - m_seq_start_time_stamp;
  SEQUENCE_EVENT& event     = m_sequence_events[ m_current_seq_num_events++ ];
  event.m_time_stamp        = event_time;
  event.m_segment           = activated_segment;
}

bool BUTTON_STRIP::update( uint32_t time_ms, uint32_t& activated_segment )
{
  bool step_triggered = false;

  switch( m_mode )
  {
    case MODE::FREE_PLAY:
    case MODE::RECORD_SEQ:
    {
      step_triggered = update_free_play( time_ms, activated_segment );

      if( m_mode == MODE::RECORD_SEQ && step_triggered )
      {
        record_sequence_event( time_ms, activated_segment );
      }
      break;
    }
    case MODE::PLAY_SEQ:
    {
      break;
    }      
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

void BUTTON_STRIP::record_sequence( uint32_t time_ms )
{
  m_mode                    = MODE::RECORD_SEQ;
  m_current_seq_event       = 0;
  m_seq_start_time_stamp    = time_ms;
}

void BUTTON_STRIP::stop_record_seqence( uint32_t time_ms )
{
  // record the final event
  record_sequence_event( time_ms, -1 );    
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

