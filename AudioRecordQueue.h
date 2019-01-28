// Based on the Teensy library AudioRecordQueue but is not an AudioStream, it is designed to be used within audio streams
// https://github.com/PaulStoffregen/Audio/blob/master/record_queue.h

#pragma once

#include <Audio.h>

// AUDIO_PRODUCER must implement
//      audio_block_t*    aquire_block_func();
//      void              release_block_func(audio_block_t* block);

template< int QUEUE_SIZE, typename AUDIO_PRODUCER >
class AUDIO_RECORD_QUEUE
{  
public:

  AUDIO_RECORD_QUEUE( AUDIO_PRODUCER& audio_producer ) :
    m_audio_producer(audio_producer),
    m_queue(),
    m_user_block(nullptr),
    m_head(0),
    m_tail(0),
    m_enabled(false)
  {
    
  }

  void start()
  {
    clear();
    m_enabled = true;    
  }
  
  void stop()
  {
    m_enabled = false;
  }
  
  int available() const
  {
    if( m_head >= m_tail )
    {
      return m_head - m_tail;
    }
    
    return QUEUE_SIZE + m_head - m_tail;    
  }
  
  void clear()
  {
    if( m_user_block != nullptr )
    {
      m_audio_producer.release_block_func( m_user_block );
      m_user_block = nullptr;
    }
  
    while( m_tail != m_head )
    {
      if( ++m_tail >= QUEUE_SIZE )
      {
        m_tail = 0;
      }
      m_audio_producer.release_block_func( m_queue[m_tail] );
    }    
  }
  
  int16_t* read_buffer()
  {
    if( m_user_block != nullptr )
    {
      Serial.println("AUDIO_RECORD_QUEUE::read_buffer() ERROR m_user_block is non-null");
      return nullptr;
    }
  
    if( m_tail == m_head )
    {
      // queue full
      Serial.println("AUDIO_RECORD_QUEUE::read_buffer() ERROR queue full");
      return nullptr;
    }
    
    if( ++m_tail >= QUEUE_SIZE )
    {
      m_tail = 0;
    }
    
    m_user_block = m_queue[m_tail];
  
    return m_user_block->data;    
  }
  
  void release_buffer()
  {
    if( m_user_block == nullptr )
    {
      Serial.println("AUDIO_RECORD_QUEUE::free_buffer() ERROR m_user_block is non-null");
      return;
    }
  
    m_audio_producer.release_block_func( m_user_block );
  
    m_user_block = nullptr;    
  }

  void update()
  {
    audio_block_t* block = m_audio_producer.aquire_block_func();
  
    if( block == nullptr )
    {
      Serial.println("null block in record queue");
      return;
    }
  
    if( !m_enabled )
    {
      // don't need to store it when not recording
      Serial.println("DISCARD");
      m_audio_producer.release_block_func( block );
      return;
    }
  
    ++m_head;
  
    if( m_head >= QUEUE_SIZE )
    {
      m_head = 0;
    }
  
    if( m_head == m_tail )
    {
      m_audio_producer.release_block_func( block );
    }
    else
    {
      m_queue[m_head] = block;
    }    
  }

private:

  AUDIO_PRODUCER&   m_audio_producer;
  audio_block_t*    m_queue[ QUEUE_SIZE ];
  audio_block_t*    m_user_block;
  volatile uint32_t m_head;
  volatile uint32_t m_tail;
  volatile bool     m_enabled;
};

