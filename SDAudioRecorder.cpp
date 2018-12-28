#include <limits>

#include "Util.h"
#include "SDAudioRecorder.h"

constexpr const char* RECORDING_FILENAME1 = "RECORD1.RAW";
constexpr const char* RECORDING_FILENAME2 = "RECORD2.RAW";

SD_AUDIO_RECORDER::SD_AUDIO_RECORDER() :
  AudioStream( 1, m_input_queue_array ),
  m_overdub_block( nullptr ),
  m_mode( MODE::STOP ),
  m_play_back_filename( RECORDING_FILENAME1 ),
  m_record_filename( RECORDING_FILENAME1 ),
  m_recorded_audio_file(),
  m_play_back_audio_file(),
  m_play_back_file_size(0),
  m_play_back_file_offset(0),
  m_jump_position(0),
  m_jump_pending(false),
  m_overdub_end_pending(false),
  m_looping(false),
  m_sd_record_queue(*this)
{

}

void SD_AUDIO_RECORDER::update()
{  
  //Serial.print( "MODE:" );
  //Serial.println( mode_to_string( m_mode ) );
  
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
        __disable_irq();

        Serial.println("Play - loop");

        start_playing();
        m_mode = MODE::PLAY;
        
        __enable_irq();
      }
      break; 
    }
    case MODE::RECORD:
    {
      update_recording();
      break;
    }
    case MODE::OVERDUB:
    {      
      update_playing();
      update_recording();

      if( m_looping && m_mode == MODE::STOP )
      {
        __disable_irq();

        switch_play_record_buffers();

        if( m_overdub_end_pending )
        {
          Serial.println("STOPPING pending overdub");
          m_mode = MODE::OVERDUB; // better to remove mode check safeguard from stop_recording()?
          stop_recording();
          start_playing();
          m_mode = MODE::PLAY;
        }
        else
        {
          start_overdub();
          m_mode = MODE::OVERDUB;
        }

        __enable_irq();
      }
    }
    default:
    {
      break;
    }
  }

  // update after updating play to capture buffer for overdub
  m_sd_record_queue.update();
}

SD_AUDIO_RECORDER::MODE SD_AUDIO_RECORDER::mode() const
{
  return m_mode;
}
  
void SD_AUDIO_RECORDER::play()
{
  Serial.println("SD_AUDIO_RECORDER::play()");

  AudioNoInterrupts();
  
  if( m_mode == MODE::OVERDUB )
  {
    stop_overdub();
  }
  else
  {
    play_file( m_play_back_filename, true );
  }

  AudioInterrupts();
}

void SD_AUDIO_RECORDER::play_file( const char* filename, bool loop )
{
  // NOTE - should this be delaying call to start_playing() until update?
  m_play_back_filename = filename;
  m_looping = loop;

  if( m_mode != MODE::PLAY )
  {
    Serial.println("Stop play named file");
    stop_current_mode( false );
  }

  __disable_irq();
  
  if( start_playing() )
  {
    m_mode = MODE::PLAY;
  }
  else
  {
    m_mode = MODE::STOP;
  }
  
  __enable_irq();
}

void SD_AUDIO_RECORDER::stop()
{
  AudioNoInterrupts();
  
  Serial.print("SD_AUDIO_RECORDER::stop() ");
  Serial.println( mode_to_string(m_mode) );
  
  stop_current_mode( true );

  m_mode = MODE::STOP;

  AudioInterrupts();
}

void SD_AUDIO_RECORDER::record()
{
  AudioNoInterrupts();
  
  if( m_mode != MODE::RECORD )
  {
    Serial.println("Stop Record");
    stop_current_mode( false );
  }

  __disable_irq();
  start_recording();
  __enable_irq();
  
  m_mode = MODE::RECORD;

  AudioInterrupts();
}

void SD_AUDIO_RECORDER::overdub()
{
  AudioNoInterrupts();
  
  ASSERT_MSG( m_mode == MODE::PLAY, "Can only overdub when playing" );

  if( m_mode == MODE::PLAY )
  {
    Serial.println("Start overdub");
    
    start_overdub();

    m_mode = MODE::OVERDUB;
  }

  AudioInterrupts();
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
  if( m_mode == MODE::OVERDUB )
  {
    ASSERT_MSG( m_overdub_block != nullptr, "Cannot overdub, no block" ); // can it be null if overdub exceeds original play file?
    audio_block_t* in_block = receiveWritable();
    //ASSERT_MSG( in_block != nullptr, "Overdub - unable to receive block" );

    // mix incoming audio with recorded audio ( from update_playing() ) then release
    if( in_block != nullptr && m_overdub_block != nullptr )
    {
      for( int i = 0; i < AUDIO_BLOCK_SAMPLES; ++i )
      {
        // TODO apply soft clipping?
        in_block->data[i] += m_overdub_block->data[i];
        ASSERT_MSG( in_block->data[i] < std::numeric_limits<int16_t>::max() && in_block->data[i] > std::numeric_limits<int16_t>::min(), "CLIPPING" );
      }
    }

    if( m_overdub_block != nullptr )
    {
      release( m_overdub_block );
      m_overdub_block = nullptr;
    }

    return in_block;
  }
  else
  {
    audio_block_t* in_block = receiveReadOnly();
    //ASSERT_MSG( in_block != nullptr, "Play - unable to receive block" );
    return in_block;
  }
}

void SD_AUDIO_RECORDER::release_block_func(audio_block_t* block)
{
  release(block);
}

bool SD_AUDIO_RECORDER::start_playing()
{
  Serial.println("SD_AUDIO_RECORDER::start_playing");
  
  // copied from https://github.com/PaulStoffregen/Audio/blob/master/play_sd_raw.cpp
  stop_playing();

  enable_SPI_audio();

  m_play_back_audio_file = SD.open( m_play_back_filename );
  
  if( !m_play_back_audio_file )
  {
    Serial.print("Unable to open file: ");
    Serial.println( m_play_back_filename );
#if defined(HAS_KINETIS_SDHC)
      if (!(SIM_SCGC3 & SIM_SCGC3_SDHC)) AudioStopUsingSPI();
#else
      AudioStopUsingSPI();
#endif

    return false;
  }

  Serial.print("Play File loaded ");
  Serial.println(m_play_back_filename);
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
    Serial.println( "Failed to allocate" );
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
    Serial.println("File End");
    m_play_back_audio_file.close();
    
    disable_SPI_audio();
    
    m_mode = MODE::STOP;
  }

  if( m_mode == MODE::OVERDUB )
  {
    ASSERT_MSG( m_overdub_block == nullptr, "Leaking overdub block" );
    m_overdub_block = block;
  }
  else
  {
    release(block);
  }
}

void SD_AUDIO_RECORDER::stop_playing()
{
  Serial.println("SD_AUDIO_RECORDER::stop_playing");
  
  if( m_mode == MODE::PLAY || m_mode == MODE::OVERDUB )
  {
    m_mode = MODE::STOP;
    
    m_play_back_audio_file.close();
    
    disable_SPI_audio();
  }
}

void SD_AUDIO_RECORDER::start_recording()
{  
  Serial.print("SD_AUDIO_RECORDER::start_recording ");
  Serial.println(m_record_filename);
  if( SD.exists( m_record_filename ) )
  {
    // delete previously existing file (SD library will append to the end)
    SD.remove( m_record_filename ); 
  } 
  
  m_recorded_audio_file = SD.open( m_record_filename, FILE_WRITE );

  if( m_recorded_audio_file )
  {
    m_sd_record_queue.start();
    Serial.print("Start recording: ");
    Serial.println( m_record_filename );
  }
  else
  {
    Serial.print("Unable to open file: ");
    Serial.println( m_record_filename );
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
  }
}

void SD_AUDIO_RECORDER::stop_recording()
{
  Serial.println("SD_AUDIO_RECORDER::stop_recording");
  m_sd_record_queue.stop();

  if( m_mode == MODE::RECORD || m_mode == MODE::OVERDUB )
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

void SD_AUDIO_RECORDER::start_overdub()
{  
  if( m_play_back_filename == RECORDING_FILENAME1 )
  {
    m_play_back_filename  = RECORDING_FILENAME1;
    m_record_filename     = RECORDING_FILENAME2;
  }
  else
  {
    m_play_back_filename  = RECORDING_FILENAME2;
    m_record_filename     = RECORDING_FILENAME1;    
  }
  
  start_playing();
  start_recording();
}

void SD_AUDIO_RECORDER::stop_overdub()
{ 
  m_overdub_end_pending = true;
}

void SD_AUDIO_RECORDER::stop_current_mode( bool reset_play_file )
{
  __disable_irq();
  
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
    case MODE::OVERDUB:
    {
      stop_overdub();
      break;
    }
    default:
    {
      break;
    }
  }

  if( reset_play_file )
  {
    m_play_back_filename = m_record_filename = RECORDING_FILENAME1;
  }

  __enable_irq();
}

void SD_AUDIO_RECORDER::switch_play_record_buffers()
{
  // toggle record/play filenames
  swap( m_play_back_filename, m_record_filename );

  Serial.print( "switch_play_record_buffers() Play: ");
  Serial.print( m_play_back_filename );
  Serial.print(" Record: " );
  Serial.println( m_record_filename );
}

const char* SD_AUDIO_RECORDER::mode_to_string( MODE mode )
{
  switch( mode )
  {
    case MODE::PLAY:
    {
      return "PLAY";
    }
    case MODE::STOP:
    {
      return "STOP";
    }
    case MODE::RECORD:
    {
      return "RECORD";
    }
    case MODE::OVERDUB:
    {
      return "OVERDUB";
    }
    default:
    {
      return nullptr;
    }
  }
}

uint32_t SD_AUDIO_RECORDER::play_back_file_time_ms() const
{
  const uint64_t num_samples = m_play_back_file_size / 2;
  const uint64_t time_in_ms = ( num_samples * 1000 ) / AUDIO_SAMPLE_RATE;

  Serial.print("Play back time in seconds:");
  Serial.println(time_in_ms / 1000.0f);

  return time_in_ms;
}

