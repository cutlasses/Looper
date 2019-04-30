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
    RECORD_INITIAL,       // record the original loop
    RECORD_PLAY,          // duplicate the loop without overdubbing
    RECORD_OVERDUB,       // overdub incoming audio onto the loop
    NONE,                 // no mode (used by pending mode)
  };

  SD_AUDIO_RECORDER();

  virtual void        update() override;

  void                update_main_loop();    // this is called outside the audio library update() which is interrupt driven
                                             // the relatively slow SD operations should be performed here

  MODE                mode() const;
  
  void                play();
  void                play_file( const char* filename, bool loop );
  void                stop();
  void                start_record();
  void                stop_record();

  bool                mode_pending() const;

  void                set_read_position( float t );
  
  uint32_t            play_back_file_time_ms() const;

  static const char*  mode_to_string( MODE mode );

  void                set_saturation( float saturation );

  //// For AUDIO_RECORD_QUEUE
  void                release_block_func(audio_block_t* block);

private:

  audio_block_t*      m_input_queue_array[1];
  audio_block_t*      m_just_played_block;   // block which was just played from the SD file

  MODE                m_mode;
  MODE                m_pending_mode;         // used to switch modes at the loop point
  const char*         m_play_back_filename;
  const char*         m_record_filename;

  File                m_recorded_audio_file;
  File                m_play_back_audio_file;
  uint32_t            m_play_back_file_size;
  uint32_t            m_play_back_file_offset;

  uint32_t            m_jump_position;
  bool                m_jump_pending;

  bool                m_looping;
  bool                m_finished_playback;

  float               m_soft_clip_coefficient;

  static constexpr const int PLAY_QUEUE_SIZE              = 64;
  static constexpr const int RECORD_QUEUE_SIZE            = 53; // matches the teensy audio library
  static constexpr const int INITIAL_PLAY_BLOCKS          = 16;
  static constexpr const int MIN_PREFERRED_PLAY_BLOCKS    = 4;
  static constexpr const int MAX_PREFERRED_RECORD_BLOCKS  = 40;
  AUDIO_RECORD_QUEUE<PLAY_QUEUE_SIZE, SD_AUDIO_RECORDER>    m_sd_play_queue;
  AUDIO_RECORD_QUEUE<RECORD_QUEUE_SIZE, SD_AUDIO_RECORDER>  m_sd_record_queue;

  audio_block_t*      create_record_block();

  // X_sd functions access the SD card - therefore should not be called within the update() interrupt
  void                start_recording_sd();
  void                update_recording_sd();
  void                stop_recording_sd( bool write_remaining_blocks = true );

  bool                start_playing_sd();
  bool                update_playing_sd();
  void                stop_playing_sd();

  void                update_playing_interrupt();

  void                stop_current_mode( bool reset_play_file );

  void                switch_play_record_buffers();

  int16_t             soft_clip_sample( int16_t sample ) const;

  inline bool         is_recording()
  {
    return m_mode == MODE::RECORD_INITIAL || m_mode == MODE::RECORD_PLAY || m_mode == MODE::RECORD_OVERDUB;           
  }

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

