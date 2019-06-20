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

 bool BUTTON_STRIP::update_steps( uint32_t time_ms, int overridden_segment )
 {
  if( overridden_segment < 0 ) // segment not overriden
  {
    if( time_ms > m_next_step_time_stamp_ms )
    {
      if( ++m_step_num >= NUM_SEGMENTS )
      {
        m_step_num = 0;
      }
  
      m_next_step_time_stamp_ms = time_ms + m_step_length_ms;
  
      return true;
    }
  }
  else if( overridden_segment != m_step_num )
  {
    m_step_num = overridden_segment;

    m_next_step_time_stamp_ms = time_ms + m_step_length_ms;
  
    return true;
  }
  
   return false;
 }

bool BUTTON_STRIP::update_free_play( uint32_t time_ms, uint32_t& activated_segment, int overridden_segment )
{
  bool step_triggered = false;
  bool update_leds    = update_steps( time_ms, overridden_segment );
  
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

bool BUTTON_STRIP::update_play_sequence( uint32_t time_ms, uint32_t& activated_segment )
{
  bool update_leds    = update_steps( time_ms, -1 );
  bool step_triggered = false;
  
  const SEQUENCE_EVENT& current_event = m_sequence_events[m_current_seq_event];

  if( time_ms - m_seq_start_time_stamp >= current_event.m_time_stamp )
  {
    DEBUG_TEXT("triggered event:");
    DEBUG_TEXT_LINE(current_event.m_segment);

    m_step_num                = current_event.m_segment;
    activated_segment         = current_event.m_segment;
    step_triggered            = true;
    update_leds               = true;
    m_next_step_time_stamp_ms = time_ms + m_step_length_ms;
    
    if( ++m_current_seq_event == m_seq_num_events )
    {
      // loop the sequence
      m_current_seq_event     = 0;
      m_seq_start_time_stamp  = time_ms;
    }
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
  Serial.print("record_sequence_event:");
  Serial.println(activated_segment);
  
  // TODO end the sequence (with final event) when it gets full
  ASSERT_MSG( m_seq_num_events < MAX_SEQUENCE_EVENTS, "Max events exceeded" );
  if( m_seq_num_events >= MAX_SEQUENCE_EVENTS )
  {
    // no space
    return;
  }
  
  const uint32_t event_time = time_ms - m_seq_start_time_stamp;
  SEQUENCE_EVENT& event     = m_sequence_events[ m_seq_num_events++ ];
  event.m_time_stamp        = event_time;
  event.m_segment           = activated_segment;
}

void BUTTON_STRIP::clear_sequence()
{
  m_seq_num_events          = 0;
  m_current_seq_event       = 0;  
}

bool BUTTON_STRIP::update( uint32_t time_ms, uint32_t& activated_segment, int overridden_segment )
{
  bool step_triggered = false;

  switch( m_mode )
  {
    case MODE::FREE_PLAY:
    case MODE::RECORD_SEQ:
    {
      step_triggered = update_free_play( time_ms, activated_segment, overridden_segment );

      if( m_mode == MODE::RECORD_SEQ && step_triggered )
      {
        record_sequence_event( time_ms, activated_segment );
      }
      break;
    }
    case MODE::PLAY_SEQ:
    {
      step_triggered = update_play_sequence( time_ms, activated_segment );
      
      break;
    }      
  }
  return step_triggered;
}

void BUTTON_STRIP::start_free_play_sequence( uint32_t sequence_length_ms, uint32_t current_time_ms )
{  
  m_mode                    = MODE::FREE_PLAY;

  clear_sequence();
  
  m_step_length_ms          = sequence_length_ms / NUM_SEGMENTS;
    
  m_running                 = true;
  m_step_num                = NUM_SEGMENTS; // next update will move to step 0
  m_next_step_time_stamp_ms = current_time_ms;
}

void BUTTON_STRIP::start_record_sequence( uint32_t time_ms )
{
  Serial.println("start_record_sequence");

  clear_sequence();
  
  m_mode                    = MODE::RECORD_SEQ;
  m_seq_start_time_stamp    = time_ms;

  m_initial_seq_segment     = m_step_num;
}

void BUTTON_STRIP::start_sequence_playback( uint32_t time_ms )
{
  Serial.println("start_sequence_playback");
  
  m_mode                    = MODE::PLAY_SEQ;
  
  // record the final event
  ++m_seq_num_events;
  record_sequence_event( time_ms, m_initial_seq_segment );    

  m_current_seq_event       = 0;
  m_seq_start_time_stamp    = time_ms; // TODO store time offset from beginning of current segement
}

void BUTTON_STRIP::stop_sequence()
{
  m_running = false;
  m_mode    = MODE::FREE_PLAY;
}

BUTTON_STRIP::MODE BUTTON_STRIP::mode() const
{
  return m_mode;
}

void BUTTON_STRIP::lock_buttons( bool lock )
{
  m_buttons_locked = lock;
}

int BUTTON_STRIP::num_segments() const
{
  return NUM_SEGMENTS;
}
