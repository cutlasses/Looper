#include "SDAudioRecorder.h"

constexpr const char* RECORDING_FILENAME = "RECORD.RAW";

SD_AUDIO_RECORDER::SD_AUDIO_RECORDER() :
  AudioStream( 1, m_input_queue_array ),
  m_mode( MODE::STOP ),
  m_recorded_audio(),
  m_sd_record_queue(),
  m_sd_play_back() 
{

}

void SD_AUDIO_RECORDER::update()
{
  
}
  
void SD_AUDIO_RECORDER::play()
{
  m_playback_file = RECORDING_FILENAME;

  m_mode = MODE::PLAY;

  start_playing();
}

void SD_AUDIO_RECORDER::play_file( const char* filename )
{
  m_playback_file = filename;

  m_mode = MODE::PLAY;
}

void SD_AUDIO_RECORDER::stop()
{
  stop_playing();

  m_mode = MODE::STOP;
}

void SD_AUDIO_RECORDER::record()
{
  start_recording();
  
  m_mode = MODE::RECORD;
}

void SD_AUDIO_RECORDER::start_playing()
{
  // NOTE - SD PLAY BACK NEEDS CONNECTING TO AUDIO GRAPH
  // replace with code from sd playback directly
  m_sd_play_back.play( m_playback_file );
}

void SD_AUDIO_RECORDER::update_playing()
{
  if( !m_sd_play_back.isPlaying() )
  {
    stop_playing();

    m_mode = MODE::STOP;
  }
}

void SD_AUDIO_RECORDER::stop_playing()
{
  m_sd_play_back.stop();
}

void SD_AUDIO_RECORDER::start_recording()
{
  if( SD.exists( RECORDING_FILENAME ) )
  {
    // delete previously existing file (SD library will append to the end)
    SD.remove( RECORDING_FILENAME ); 
  } 
  
  m_recorded_audio = SD.open( RECORDING_FILENAME, FILE_WRITE );

  if( m_recorded_audio )
  {
    m_sd_record_queue.begin();
  }  
}

void SD_AUDIO_RECORDER::update_recording()
{
  
}

void SD_AUDIO_RECORDER::stop_recording()
{
  m_sd_record_queue.end();
}

