#pragma once

#include <Audio.h>
#include "Util.h"

class SOFT_CLIP : public AudioStream
{ 
public:

  SOFT_CLIP() :
    AudioStream( 1, m_input_queue_array ),
    m_soft_clip_coefficient(0.0f)
  {
      
  }

  virtual void        update() override
  {
    audio_block_t* in_block = receiveWritable();

    if( in_block != nullptr )
    {
      for( int i = 0; i < AUDIO_BLOCK_SAMPLES; ++i )
      {
        ASSERT_MSG( in_block->data[i] < std::numeric_limits<int16_t>::max() && in_block->data[i] > std::numeric_limits<int16_t>::min(), "PRE CLIPPING" );
        in_block->data[i] = DSP_UTILS::soft_clip_sample( in_block->data[i], m_soft_clip_coefficient );
        ASSERT_MSG( in_block->data[i] < std::numeric_limits<int16_t>::max() && in_block->data[i] > std::numeric_limits<int16_t>::min(), "POST CLIPPING" );
      }
      
      transmit( in_block, 0 );
      release( in_block );
    }
  }

  void set_saturation( float saturation )
  {
    constexpr const float MIN_SATURATION = 0.0f;
    constexpr const float MAX_SATURATION = 1.0f / 3.0f;
  
    m_soft_clip_coefficient = lerp( MIN_SATURATION, MAX_SATURATION, saturation );
  }

private:

  audio_block_t*      m_input_queue_array[1];
  float               m_soft_clip_coefficient;
};
