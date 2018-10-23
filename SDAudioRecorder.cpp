#include "SDAudioRecorder.h"

constexpr const char* RECORDING_FILENAME = "RECORD.RAW";

SD_AUDIO_RECORDER::SD_AUDIO_RECORDER() :
  AudioStream( 1, m_input_queue_array ),
  m_mode( MODE::STOP ),
  m_recorded_audio_file(),
  m_play_back_audio_file(),
  m_play_back_file_size(0),
  m_play_back_file_offset(0),
  m_jump_position(0),
  m_jump_pending(false),
  m_looping(false),
  m_sd_record_queue(*this)
{

}

void SD_AUDIO_RECORDER::update()
{
  m_sd_record_queue.update(); // NOTE - thus is UNCONNECTED
  
  switch( m_mode )
  {
    case MODE::PLAY:
    {
      if( m_jump_pending )
      {
        if( m_play_back_audio_file.seek( m_jump_position ) )
        {
          m_jump_pending = false;
          m_play_back_file_offset = m_jump_position;
        }
      }
      
      update_playing();

      if( m_looping && m_mode == MODE::STOP )
      {
        start_playing();
        m_mode = MODE::PLAY;
      }
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

SD_AUDIO_RECORDER::MODE SD_AUDIO_RECORDER::mode() const
{
  return m_mode;
}
  
void SD_AUDIO_RECORDER::play()
{
  m_play_back_file = RECORDING_FILENAME;

  play_file( RECORDING_FILENAME, true );
}

void SD_AUDIO_RECORDER::play_file( const char* filename, bool loop )
{
  // NOTE - should this be delaying call to start_playing() until update?
  m_play_back_file = filename;
  m_looping = loop;

  if( m_mode != MODE::PLAY )
  {
    stop();
  }

  if( start_playing() )
  {
    m_mode = MODE::PLAY;
  }
  else
  {
    m_mode = MODE::STOP;
  }
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
  if( m_mode != MODE::RECORD )
  {
    stop();
  }
    
  start_recording();
  
  m_mode = MODE::RECORD;
}

void SD_AUDIO_RECORDER::set_read_position( float t )
{
 if( m_mode == MODE::PLAY )
 {
  const uint32_t block_size   = 2; // AUDIO_BLOCK_SAMPLES
  const uint32_t file_pos     = m_play_back_file_size * t;
  const uint32_t block_rem    = file_pos % block_size;
  
  
  m_jump_pending  = true;
  m_jump_position = file_pos + block_rem;
 }
}

audio_block_t* SD_AUDIO_RECORDER::aquire_block_func()
{
  return receiveReadOnly();
}

void SD_AUDIO_RECORDER::release_block_func(audio_block_t* block)
{
  release(block);
}

bool SD_AUDIO_RECORDER::start_playing()
{
  // copied from https://github.com/PaulStoffregen/Audio/blob/master/play_sd_raw.cpp
  stop_playing();
#if defined(HAS_KINETIS_SDHC)
  if (!(SIM_SCGC3 & SIM_SCGC3_SDHC)) AudioStartUsingSPI();
#else
  AudioStartUsingSPI();
#endif
  __disable_irq();
  m_play_back_audio_file = SD.open( m_play_back_file );
  __enable_irq();
  
  if( !m_play_back_audio_file )
  {
    Serial.println("Unable to open file");
#if defined(HAS_KINETIS_SDHC)
      if (!(SIM_SCGC3 & SIM_SCGC3_SDHC)) AudioStopUsingSPI();
#else
      AudioStopUsingSPI();
#endif

    return false;
  }

  Serial.print("File loaded ");
  Serial.println(m_play_back_file);
  m_play_back_file_size = m_play_back_audio_file.size();
  m_play_back_file_offset = 0;
  Serial.print("File open - file size: ");
  Serial.println(m_play_back_file_size);

  return true;
}

void SD_AUDIO_RECORDER::update_playing()
{
  audio_block_t *block;

  // allocate the audio blocks to transmit
  block = allocate();
  if( block == nullptr )
  {
    return;
  }

  if( m_play_back_audio_file.available() )
  {
    // we can read more data from the file...
    const uint32_t n = m_play_back_audio_file.read( block->data, AUDIO_BLOCK_SAMPLES*2 );
    m_play_back_file_offset += n;
    for( int i = n/2; i < AUDIO_BLOCK_SAMPLES; i++ )
    {
      block->data[i] = 0;
    }
    transmit(block);
  }
  else
  {
    m_play_back_audio_file.close();
    
#if defined(HAS_KINETIS_SDHC)
      if (!(SIM_SCGC3 & SIM_SCGC3_SDHC)) AudioStopUsingSPI();
#else
      AudioStopUsingSPI();
#endif
    m_mode = MODE::STOP;
  }
  
  release(block);
}

void SD_AUDIO_RECORDER::stop_playing()
{
  __disable_irq();
  
  if( m_mode == MODE::PLAY )
  {
    m_mode = MODE::STOP;
    
    __enable_irq();
    m_play_back_audio_file.close();
    
#if defined(HAS_KINETIS_SDHC)
      if (!(SIM_SCGC3 & SIM_SCGC3_SDHC)) AudioStopUsingSPI();
#else
      AudioStopUsingSPI();
#endif
  }
  else
  {
    __enable_irq();
  }
}

void SD_AUDIO_RECORDER::start_recording()
{
  Serial.println("Start recording");
  if( SD.exists( RECORDING_FILENAME ) )
  {
    // delete previously existing file (SD library will append to the end)
    SD.remove( RECORDING_FILENAME ); 
  } 
  
  m_recorded_audio_file = SD.open( RECORDING_FILENAME, FILE_WRITE );

  if( m_recorded_audio_file )
  {
    m_sd_record_queue.start();
    Serial.println("Queue begin");
  }  
}

void SD_AUDIO_RECORDER::update_recording()
{
  if( m_sd_record_queue.available() >= 2 )
  {
    byte buffer[512]; // arduino library most efficient with full 512 sector size writes

    // write 2 x 256 byte blocks to buffer
    memcpy( buffer, m_sd_record_queue.read_buffer(), 256);
    m_sd_record_queue.release_buffer();
    memcpy( buffer + 256, m_sd_record_queue.read_buffer(), 256);
    m_sd_record_queue.release_buffer();

    m_recorded_audio_file.write( buffer, 512 );

    Serial.println("Data written");
  }
}

void SD_AUDIO_RECORDER::stop_recording()
{
  Serial.println("Stop recording");
  m_sd_record_queue.stop();

  if( m_mode == MODE::RECORD )
  {
    // empty the record queue
    while( m_sd_record_queue.available() > 0 )
    {
      m_recorded_audio_file.write( reinterpret_cast<byte*>(m_sd_record_queue.read_buffer()), 256 );
      m_sd_record_queue.release_buffer();
    }

    m_recorded_audio_file.close();
  }

  m_mode = MODE::STOP;
}

