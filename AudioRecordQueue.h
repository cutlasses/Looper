// Based on the Teensy library AudioRecordQueue but is not an AudioStream, it is designed to be used within audio streams
// https://github.com/PaulStoffregen/Audio/blob/master/record_queue.h

#pragma once

#include <Audio.h>

// AUDIO_PRODUCER must implement
//      void              release_block_func(audio_block_t* block);

template< int QUEUE_SIZE, typename AUDIO_PRODUCER >
class AUDIO_RECORD_QUEUE
{  
public:

  AUDIO_RECORD_QUEUE( AUDIO_PRODUCER& audio_producer, const char* queue_name ) :
    m_audio_producer(audio_producer),
    m_queue_name( queue_name ),
    m_queue(),
    m_user_block(nullptr),
    m_head(0),
    m_tail(0),
    m_enabled(false)
  {
    
  }

  void debug_log_stats() const
  {
    Serial.print( "AUDIO_RECORD_QUEUE::debug_log_stats() " );
    Serial.print( m_queue_name );
    Serial.print( " Size:" );
    Serial.print( size() );  
    Serial.print( " Remaining:" );
    Serial.println( remaining() );       
  }

  void start()
  {
    Serial.println( "AUDIO_RECORD_QUEUE::start" );
    clear();
    m_enabled = true;    
  }
  
  void stop()
  {
    Serial.println( "AUDIO_RECORD_QUEUE::stop" );
    m_enabled = false;
  }
  
  int size() const
  {
    if( m_head >= m_tail )
    {
      return m_head - m_tail;
    }
    
    return QUEUE_SIZE + m_head - m_tail;    
  }

  int remaining() const
  {
    return QUEUE_SIZE - size() - 1; // can't use final block (when head == tail it removes block)
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

  audio_block_t* read_block()
  {
    if( m_user_block != nullptr )
    {
      Serial.println("AUDIO_RECORD_QUEUE::read_buffer() ERROR m_user_block is non-null");
      return nullptr;
    }

    uint32_t t = m_tail;
  
    if( t == m_head )
    {
      // queue full
      Serial.print("AUDIO_RECORD_QUEUE::read_buffer() ERROR queue empty ");
      Serial.println( m_queue_name );
      return nullptr;
    }
    
    if( ++t >= QUEUE_SIZE )
    {
      t = 0;
    }
    
    m_user_block = m_queue[t];

    m_tail = t;

    return m_user_block;
  }
  
  int16_t* read_buffer()
  {
    return read_block()->data;    
  }
  
  void release_buffer( bool free_block = true )
  {
    if( m_user_block == nullptr )
    {
      Serial.println("AUDIO_RECORD_QUEUE::free_buffer() ERROR m_user_block is non-null");
      return;
    }

    if( free_block )
    {
      m_audio_producer.release_block_func( m_user_block );
    }
  
    m_user_block = nullptr;    
  }

  void update( audio_block_t* block )
  {   
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
  
    uint32_t h = m_head + 1;
  
    if( h >= QUEUE_SIZE )
    {
      h = 0;
    }
  
    if( h == m_tail )
    {
      Serial.print("AUDIO_RECORD_QUEUE::update() QUEUE FULL, RELEASING BLOCK:");
      Serial.println((int)block, HEX);
      debug_log_stats();
      m_audio_producer.release_block_func( block );
    }
    else
    {
      m_queue[h] = block;
      m_head = h;
    }
  }

private:

  AUDIO_PRODUCER&   m_audio_producer;
  const char*       m_queue_name;
  audio_block_t*    m_queue[ QUEUE_SIZE ];
  audio_block_t*    m_user_block;
  volatile uint32_t m_head;
  volatile uint32_t m_tail;
  volatile bool     m_enabled;
};

