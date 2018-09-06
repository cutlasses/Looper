#pragma once

#include <Audio.h>

class SD_AUDIO_RECORDER : public AudioStream
{
    audio_block_t*  m_input_queue_array[1];
  
public:

  SD_AUDIO_RECORDER();

  virtual void      update() override;
  
  void              play();
  void              stop();
  void              record();

private:

  AudioRecordQueue  sd_record_queue;      // need to expose these for the AudioConnection
  AudioPlaySdRaw    sd_play_back;
};

