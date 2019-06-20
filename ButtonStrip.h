#pragma once

#include <stdint.h>


class BUTTON_STRIP
{
public:

  enum class MODE
  {
    FREE_PLAY,
    RECORD_SEQ,
    PLAY_SEQ
  };
  
private:
  static const constexpr int NUM_SEGMENTS               = 8;
  static const constexpr int32_t BUTTON_DEBOUNCE_MS     = 5;
  static const constexpr int64_t LED_I2C_UPDATE_TIME_MS = 30;
  static const constexpr int MAX_SEQUENCE_EVENTS        = 32;

  struct DEBOUNCE_DETAILS
  {
    bool            m_button_down               = false;
    bool            m_registered                = false;
    uint32_t        m_time_stamp                = 0;
  };

  struct SEQUENCE_EVENT
  {
    uint32_t        m_time_stamp                = 0;
    int16_t         m_segment                   = 0;
  };
  
  const int         m_i2c_address; 
  uint8_t           m_switch_values             = 0;
  uint8_t           m_step_num                  = 0;
  
  uint32_t          m_step_length_ms            = 0;
  uint32_t          m_next_step_time_stamp_ms   = 0;

  MODE              m_mode                      = MODE::FREE_PLAY;

  bool              m_running                   = false;
  bool              m_buttons_locked            = false;

  DEBOUNCE_DETAILS  m_debounce_details[NUM_SEGMENTS];

  SEQUENCE_EVENT    m_sequence_events[MAX_SEQUENCE_EVENTS];
  uint16_t          m_current_seq_event         = 0;
  uint16_t          m_seq_num_events            = 0;
  uint32_t          m_seq_start_time_stamp      = 0;
  uint8_t           m_initial_seq_segment       = 0;


  void              send_led_values(uint8_t led_values);
  bool              update_steps( uint32_t time_ms, int overridden );
  bool              update_free_play( uint32_t time_ms, uint32_t& activated_segment, int overridden_segment );
  bool              update_play_sequence( uint32_t time_ms, uint32_t& activated_segment );
  void              record_sequence_event( uint32_t time_ms, int32_t activated_segment );
  
public:

  BUTTON_STRIP( int i2c_address );
  
  bool              update( uint32_t time_ms, uint32_t& activated_segment, int overridden_segment = -1 );
  
  void              start_free_play_sequence( uint32_t sequence_length_ms, uint32_t current_time_ms );
  void              start_record_sequence(uint32_t time_ms );
  void              start_sequence_playback( uint32_t time_ms );
  void              stop_sequence();
  void              clear_sequence();

  MODE              mode() const;

  void              lock_buttons( bool lock );

  int               num_segments() const;
};

