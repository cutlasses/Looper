#include <ADC.h>
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

#include "ButtonStrip.h"
#include "LooperInterface.h"
#include "SDAudioRecorder.h"

constexpr int SDCARD_CS_PIN    = BUILTIN_SDCARD;
constexpr int SDCARD_MOSI_PIN  = 11;  // not actually used CAN I DELETE?
constexpr int SDCARD_SCK_PIN   = 13;  // not actually used

constexpr int I2C_ADDRESS(0x01); 

constexpr int MAX_SAMPLES(12);
char* sample_files[MAX_SAMPLES];
int num_samples_loaded        = 0;

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

AudioMixer4       mixer;

AudioConnection   patch_cord_1( io.audio_input, 0, audio_recorder, 0 );
AudioConnection   patch_cord_2( io.audio_input, 0, mixer, 0 );
AudioConnection   patch_cord_3( audio_recorder, 0, mixer, 1 );
AudioConnection   patch_cord_4( mixer, 0, io.audio_output, 0 );

BUTTON_STRIP      button_strip( I2C_ADDRESS );

LOOPER_INTERFACE  looper_interface;

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

// find all .raw files in dir and add to sample list
void fill_sample_list( File dir )
{
  for( int i = 0; i < MAX_SAMPLES; ++i )
  {
    sample_files[i] = nullptr;  
  }
  
  while(1)
  {
    File entry = dir.openNextFile();
    if( !entry )
    {
      // done!
      return;
    }

    if( !entry.isDirectory() )
    {
      const int entry_filename_length = strlen(entry.name());
      constexpr const char* file_ext = ".RAW";
      constexpr int file_ext_length = strlen( file_ext );
      const char* entry_ext = entry.name() + entry_filename_length - file_ext_length;
      
      if( entry_filename_length > file_ext_length && strncmp( entry_ext, file_ext, file_ext_length ) == 0 )
      { 
        sample_files[num_samples_loaded] = new char[entry_filename_length+1];
        strcpy( sample_files[num_samples_loaded], entry.name() );
  
        if( ++num_samples_loaded == MAX_SAMPLES )
        {
          // sample list full
          return;
        }
      }
    }
  }
}

void setup()
{
  Serial.begin(9600);

  constexpr int mem_size = 256;
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

  File root = SD.open("/");
  fill_sample_list( root );

  Serial.println("Files:");
  for( int i = 0; i < num_samples_loaded; ++i )
  {
    Serial.println( sample_files[i] );
  }

  looper_interface.setup( num_samples_loaded );

  Wire.begin( I2C_ADDRESS );

  button_strip.set_sequence_length( 3000 ); // remove once time extracted from sample

  Serial.print("Setup finished!\n");
  delay(500);
}

void loop()
{
  uint64_t time_ms = millis();

  if( looper_interface.update( io.adc, time_ms ) )
  {
    // mode changed
    audio_recorder.stop();
  }

  switch( looper_interface.mode() )
  {
    case LOOPER_INTERFACE::MODE::SD_PLAYBACK:
    {
      int sample_index( 0 );
      if( looper_interface.sample_to_play( sample_index ) )
      {
        Serial.print("Playing:");
        Serial.print(sample_index);
        Serial.print(", ");
        Serial.println( sample_files[sample_index] );
        
        audio_recorder.play_file( sample_files[ sample_index ], true );
        button_strip.set_sequence_length( audio_recorder.play_back_file_time_ms() );
      }
      break;
    }
    case LOOPER_INTERFACE::MODE::LOOPER:
    {
      if( looper_interface.record_button().single_click() )
      {
        switch( audio_recorder.mode() )
        {
          case SD_AUDIO_RECORDER::MODE::STOP:
          case SD_AUDIO_RECORDER::MODE::PLAY:
          {
            // start recording over the top
            audio_recorder.record();
            looper_interface.set_recording( true, time_ms );
            
            Serial.println("RECORD");
            break;
          }
          case SD_AUDIO_RECORDER::MODE::RECORD:
          {
            // stop recording and play loop
            audio_recorder.play();
            looper_interface.set_recording( false, time_ms );
            button_strip.set_sequence_length( audio_recorder.play_back_file_time_ms() );
            
            Serial.println("PLAY");
            break;
          }
          default:
          {
            // further modes to come..
            Serial.println("Unknown looper mode");
            break;
          }
        }
      }
      break;
    }
    default:
    {
      Serial.println("Error:what mode is this");
      break;
    }
  }

  const float mix = looper_interface.mix();
  mixer.gain( 0, 1.0f - mix );
  mixer.gain( 1, mix );
  
  uint32_t segment;
  if( button_strip.update( time_ms, segment ) )
  {
    if( audio_recorder.mode() == SD_AUDIO_RECORDER::MODE::PLAY )
    {
      const float t = segment / static_cast<float>(button_strip.num_segments());
      audio_recorder.set_read_position( t );
    }
  }
}
