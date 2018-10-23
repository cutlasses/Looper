#include "AudioRecordQueue.h"

AUDIO_RECORD_QUEUE::AUDIO_RECORD_QUEUE() :
  m_queue(),
  m_user_block(nullptr),
  m_head(0),
  m_tail(0),
  m_enabled(false)
{
  
}

void AUDIO_RECORD_QUEUE::start()
{
  clear();
  m_enabled = true;
}

void AUDIO_RECORD_QUEUE::stop()
{
  m_enabled = false;
}

int AUDIO_RECORD_QUEUE::available()
{
  if( m_head >= m_tail )
  {
    return m_head - m_tail;
  }
  
  return QUEUE_SIZE + m_head - m_tail;
}

void AUDIO_RECORD_QUEUE::clear()
{
  if( m_user_block != nullptr )
  {
    release_block_func( m_user_block );
    m_user_block = NULL;
  }

  while( m_tail != m_head )
  {
    if( ++m_tail >= QUEUE_SIZE )
    {
      m_tail = 0;
    }
    release_block_func( m_queue[m_tail] );
  }
}

int16_t* AUDIO_RECORD_QUEUE::read_buffer()
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

void AUDIO_RECORD_QUEUE::release_buffer()
{
  if( m_user_block == nullptr )
  {
    Serial.println("AUDIO_RECORD_QUEUE::free_buffer() ERROR m_user_block is non-null");
    return;
  }

  release_block_func( m_user_block );

  m_user_block = nullptr;
}

void AUDIO_RECORD_QUEUE::update()
{
  audio_block_t* block = aquire_block_func();

  if( block == nullptr )
  {
    return;
  }

  if( !m_enabled )
  {
    // don't need to store it when not recording
    release_block_func( block );
    return;
  }

  ++m_head;

  if( m_head >= QUEUE_SIZE )
  {
    m_head = 0;
  }

  if( m_head == m_tail )
  {
    release_block_func( block );
  }
  else
  {
    m_queue[m_head] = block;
  }
}

