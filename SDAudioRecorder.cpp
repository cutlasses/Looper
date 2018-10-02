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
  switch( m_mode )
  {
    case MODE::PLAY:
    {
      update_playing();
      break; 
    }
    case MODE::RECORD:
    {
      update_recording();
      break;
    }
    default:
    {
      break;
    }
  }
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
  switch( m_mode )
  {
    case MODE::PLAY:
    {
      stop_playing();
      break; 
    }
    case MODE::RECORD:
    {
      stop_recording();
      break;
    }
    default:
    {
      break;
    }
  }

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
  if( m_sd_record_queue.available() >= 2 )
  {
    byte buffer[512]; // arduino library most efficient whith full 512 sector size writes

    // write 2 x 256 byte blocks to buffer
    memcpy( buffer, m_sd_record_queue.readBuffer(), 256);
    m_sd_record_queue.freeBuffer();
    memcpy( buffer + 256, m_sd_record_queue.readBuffer(), 256);
    m_sd_record_queue.freeBuffer();

    m_recorded_audio.write( buffer, 512 );
  }
}

void SD_AUDIO_RECORDER::stop_recording()
{
  m_sd_record_queue.end();

  if( m_mode == MODE::RECORD )
  {
    // empty the record queue
    while( m_sd_record_queue.available() > 0 )
    {
      m_recorded_audio.write( reinterpret_cast<byte*>(m_sd_record_queue.readBuffer()), 256 );
      m_sd_record_queue.freeBuffer();
    }

    m_recorded_audio.close();
  }

  m_mode = MODE::STOP;
}

