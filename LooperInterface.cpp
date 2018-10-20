#include "LooperInterface.h"
#include "Util.h"

LOOPER_INTERFACE::LOOPER_INTERFACE() :
  m_dials( { DIAL( A20 ), DIAL( A19 ), DIAL( A18 ), DIAL( A17 ), DIAL( A16 ), DIAL( A13 ) } ),
  m_mode_button( MODE_BUTTON_PIN, true ),
  m_record_button( RECORD_BUTTON_PIN, true ),
  m_leds( { LED( LED_1_PIN, false ), LED( LED_2_PIN, false ), LED( LED_3_PIN, false ) } )
{

}

void LOOPER_INTERFACE::setup()
{
  m_mode_button.setup();
  m_record_button.setup();

  for( int l = 0; l < NUM_LEDS; ++l )
  {
    m_leds[l].setup();
    m_leds[l].set_brightness( 0.25f );
  }
}

void LOOPER_INTERFACE::update( ADC& adc, uint32_t time_in_ms )
{
  // read each pot
  for( int d = 0; d < NUM_DIALS; ++d )
  {
    m_dials[d].update( adc );
  }
  
  m_mode_button.update( time_in_ms );
  m_record_button.update( time_in_ms );

  for( int l = 0; l < NUM_LEDS; ++l )
  {
    m_leds[l].update( time_in_ms );
  }
}

