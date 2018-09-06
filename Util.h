#pragma once

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

/////////////////////////////////////////////////////

template <typename T>
T lerp( const T& v1, const T& v2, float t )
{
  return v1 + ( (v2 - v1) * t );
}

/////////////////////////////////////////////////////

constexpr int trunc_to_int( float v )
{
  return static_cast<int>( trunc(v) );
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
