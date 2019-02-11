#pragma once

#include <Arduino.h>
#include "CompileSwitches.h"

#ifdef DEBUG_OUTPUT

extern bool serial_port_initialised;

inline bool _assert_fail( const char* assert, const char* msg )
{
  if( serial_port_initialised )
  {
    Serial.print(assert);
    Serial.print(" ");
    Serial.print(msg);
    Serial.print("\n");
  }
  
  return true;
}

#define ASSERT_MSG(x, msg) ((void)((x) || (_assert_fail(#x,msg))))
#define DEBUG_TEXT(x) if(serial_port_initialised) Serial.print(x);
#else
#define ASSERT_MSG(x, msg)
#define DEBUG_TEXT(x)
#endif

//#define SHOW_TIMED_SECTIONS

#ifdef SHOW_TIMED_SECTIONS
#define ADD_TIMED_SECTION(x) TIMED_SECTION __FILE__##__LINE__(x)
#else
#define ADD_TIMED_SECTION(x)
#endif

struct TIMED_SECTION
{
  const char* m_section_name;
  uint64_t    m_start_time;

  TIMED_SECTION( const char* section_name ) :
    m_section_name( section_name ),
    m_start_time( micros() )
  {      
  }

  ~TIMED_SECTION()
  {
    const uint64_t duration = micros() - m_start_time;
    Serial.print( m_section_name );
    Serial.print(" ");
    Serial.print( static_cast<int>(duration) );
    Serial.println( "us" );
  }
};

/////////////////////////////////////////////////////

template <typename T>
T clamp( const T& value, const T& min, const T& max )
{
  if( value < min )
  {
    return min;
  }
  if( value > max )
  {
    return max;
  }
  return value;
}

template <typename T>
T max_val( const T& v1, const T& v2 )
{
  if( v1 > v2 )
  {
    return v1;
  }
  else
  {
    return v2;
  }
}

template <typename T>
T min_val( const T& v1, const T& v2 )
{
  if( v1 < v2 )
  {
    return v1;
  }
  else
  {
    return v2;
  }
}

template <typename T>
void swap( T& v1, T& v2 )
{
  T temp = v1;
  v1 = v2;
  v2 = temp;
}

/////////////////////////////////////////////////////

template <typename T>
T lerp( const T& v1, const T& v2, float t )
{
  return v1 + ( (v2 - v1) * t );
}

/////////////////////////////////////////////////////

inline constexpr int trunc_to_int( float v )
{
  return static_cast<int>( v );
}

inline constexpr int round_to_int( float v )
{
  return trunc_to_int( v + 0.5f );
}

/////////////////////////////////////////////////////

template < typename TYPE, int CAPACITY >
class RUNNING_AVERAGE
{
  TYPE                    m_values[ CAPACITY ];
  int                     m_current;
  int                     m_size;

public:

  RUNNING_AVERAGE() :
    m_values(),
    m_current(0),
    m_size(0)
  {
  }

  void add( TYPE value )
  {
    m_values[ m_current ] = value;
    m_current             = ( m_current + 1 ) % CAPACITY;
    ++m_size;
    if( m_size > CAPACITY )
    {
      m_size              = CAPACITY;
    }
  }

  void reset()
  {
    m_size                = 0;
    m_current             = 0;
  }
  
  TYPE average() const
  {
    if( m_size == 0 )
    {
      return 0;  
    }
    
    TYPE avg = 0;
    for( int x = 0; x < m_size; ++x )
    {
      avg += m_values[ x ];
    }

    return avg / m_size;
  }

  int size() const
  {
    return m_size;
  }
};
