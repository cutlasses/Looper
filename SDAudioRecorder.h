#pragma once

#include <Audio.h>
#include "AudioRecordQueue.h"

class SD_AUDIO_RECORDER : public AudioStream
{
  
public:

  enum class MODE
  {
    PLAY,
    STOP,
    RECORD,
    OVERDUB,
  };

  SD_AUDIO_RECORDER();

  virtual void        update() override;

  MODE                mode() const;
  
  void                play();
  void                play_file( const char* filename, bool loop );
  void                stop();
  void                record();
  void                overdub();

  void                set_read_position( float t );
  
  uint32_t            play_back_file_time_ms() const;

  static const char*  mode_to_string( MODE mode );

  //// For AUDIO_RECORD_QUEUE
  audio_block_t*      aquire_block_func();
  void                release_block_func(audio_block_t* block);

private:

  audio_block_t*      m_input_queue_array[1];
  audio_block_t*      m_overdub_block;

  MODE                m_mode;
  const char*         m_play_back_filename;
  const char*         m_record_filename;

  File                m_recorded_audio_file;
  File                m_play_back_audio_file;
  uint32_t            m_play_back_file_size;
  uint32_t            m_play_back_file_offset;

  uint32_t            m_jump_position;
  bool                m_jump_pending;

  bool                m_switch_buffers_pending;

  bool                m_looping;
  
  static constexpr const int QUEUE_SIZE = 53; // matches the teensy audio library
  AUDIO_RECORD_QUEUE<QUEUE_SIZE, SD_AUDIO_RECORDER>  m_sd_record_queue;

  void                start_recording();
  void                update_recording();
  void                stop_recording();

  bool                start_playing();
  void                update_playing();
  void                stop_playing();

  void                start_overdub();
  void                stop_overdub();

  void                stop_current_mode( bool reset_play_file );

  void                switch_play_record_buffers();

  inline void         enable_SPI_audio()
  {
#if defined(HAS_KINETIS_SDHC)
  if (!(SIM_SCGC3 & SIM_SCGC3_SDHC)) AudioStartUsingSPI();
#else
  AudioStartUsingSPI();
#endif    
  }
  
  inline void         disable_SPI_audio()
  {
#if defined(HAS_KINETIS_SDHC)
      if (!(SIM_SCGC3 & SIM_SCGC3_SDHC)) AudioStopUsingSPI();
#else
      AudioStopUsingSPI();
#endif    
  }
};

