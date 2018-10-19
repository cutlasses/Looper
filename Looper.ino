#include <ADC.h>
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

#include "ButtonStrip.h"
#include "SDAudioRecorder.h"

constexpr int SDCARD_CS_PIN    = BUILTIN_SDCARD;
constexpr int SDCARD_MOSI_PIN  = 11;  // not actually used CAN I DELETE?
constexpr int SDCARD_SCK_PIN   = 13;  // not actually used

constexpr int I2C_ADDRESS(0x01); 

// wrap in a struct to ensure initialisation order
struct IO
{
  ADC                         adc;
  AudioInputAnalog            audio_input;
  AudioOutputAnalog           audio_output;

  IO() :
    adc(),
    audio_input(A0),
    audio_output()
  {
  }
};

IO io;

SD_AUDIO_RECORDER audio_recorder;

AudioConnection   patch_cord_1( io.audio_input, 0, audio_recorder, 0 );
AudioConnection   patch_cord_2( audio_recorder, 0, io.audio_output, 0 );

BUTTON_STRIP      button_strip( I2C_ADDRESS );

//////////////////////////////////////

void set_adc1_to_3v3()
{
  ADC1_SC3 = 0;                 // cancel calibration
  ADC1_SC2 = ADC_SC2_REFSEL(0); // vcc/ext ref 3.3v

  ADC1_SC3 = ADC_SC3_CAL;       // begin calibration

  uint16_t sum;

  while( (ADC1_SC3 & ADC_SC3_CAL))
  {
    // wait
  }

  __disable_irq();

    sum = ADC1_CLPS + ADC1_CLP4 + ADC1_CLP3 + ADC1_CLP2 + ADC1_CLP1 + ADC1_CLP0;
    sum = (sum / 2) | 0x8000;
    ADC1_PG = sum;
    sum = ADC1_CLMS + ADC1_CLM4 + ADC1_CLM3 + ADC1_CLM2 + ADC1_CLM1 + ADC1_CLM0;
    sum = (sum / 2) | 0x8000;
    ADC1_MG = sum;

  __enable_irq();
  
}

void setup()
{
  Serial.begin(9600);

  constexpr int mem_size = 256 ;
  AudioMemory( mem_size );

  analogReference(INTERNAL);

  set_adc1_to_3v3();

  // Initialize the SD card
  SPI.setMOSI(SDCARD_MOSI_PIN);
  SPI.setSCK(SDCARD_SCK_PIN);
  
  if( !( SD.begin(SDCARD_CS_PIN) ) )
  {
    // stop here if no SD card, but print a message
    while (1)
    {
      Serial.println("Unable to access the SD card");
      delay(500);
    }
  }

  Wire.begin( I2C_ADDRESS );

  button_strip.set_step_length( 300 ); // remove once time extracted from sample

  Serial.print("Setup finished!\n");
  delay(500);
}

void loop()
{
  uint64_t time_ms = millis();
  
  static bool start = false;
  if( !start )
  {
    start = true;

    audio_recorder.play_file( "drumloop.raw", true );
  }

  uint32_t segment;
  if( button_strip.update( time_ms, segment ) )
  {
    const float t = segment / static_cast<float>(button_strip.num_segments());
    //audio_recorder.set_read_position( t );
  }
}
