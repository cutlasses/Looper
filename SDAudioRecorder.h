#pragma once

#include <Audio.h>

class SD_AUDIO_RECORDER : public AudioStream
{
  enum class MODE
  {
    PLAY,
    STOP,
    RECORD,
    OVERDUB,
  };
  
public:

  SD_AUDIO_RECORDER();

  virtual void      update() override;
  
  void              play();
  void              play_file( const char* filename );
  void              stop();
  void              record();

private:

  audio_block_t*    m_input_queue_array[1];

  MODE              m_mode;
  const char*       m_playback_file;

  File              m_recorded_audio_file;
  File              m_play_back_audio_file;
  uint32_t          m_play_back_file_size;
  uint32_t          m_play_back_file_offset;
  

  AudioRecordQueue  m_sd_record_queue;      // need to expose these for the AudioConnection

  void              start_recording();
  void              update_recording();
  void              stop_recording();

  bool              start_playing();
  void              update_playing();
  void              stop_playing();
};

