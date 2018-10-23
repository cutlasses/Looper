// Based on the Teensy library AudioRecordQueue but is not an AudioStream, it is designed to be used within audio streams
// https://github.com/PaulStoffregen/Audio/blob/master/record_queue.h

#pragma once

#include <Audio.h>

class AUDIO_RECORD_QUEUE
{  
public:

  AUDIO_RECORD_QUEUE();

  void              start();
  void              stop();
  int               available();
  void              clear();
  int16_t*          read_buffer();
  void              release_buffer();

  void              update();

private:

  static constexpr const int QUEUE_SIZE = 53;

  audio_block_t*    m_queue[ QUEUE_SIZE ];
  audio_block_t*    m_user_block;
  volatile uint32_t m_head;
  volatile uint32_t m_tail;
  volatile bool     m_enabled;

  audio_block_t*    aquire_block_func()                       { return nullptr;}
  void              release_block_func(audio_block_t* block)  {;}
};

