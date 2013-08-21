#ifndef _nvsg_nvmath_nvmath_
#define _nvsg_nvmath_nvmath_

// Copyright NVIDIA Corporation 2002-2006
// TO THE MAXIMUM EXTENT PERMITTED BY APPLICABLE LAW, THIS SOFTWARE IS PROVIDED
// *AS IS* AND NVIDIA AND ITS SUPPLIERS DISCLAIM ALL WARRANTIES, EITHER EXPRESS
// OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY
// AND FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL NVIDIA OR ITS SUPPLIERS
// BE LIABLE FOR ANY SPECIAL, INCIDENTAL, INDIRECT, OR CONSEQUENTIAL DAMAGES
// WHATSOEVER (INCLUDING, WITHOUT LIMITATION, DAMAGES FOR LOSS OF BUSINESS PROFITS,
// BUSINESS INTERRUPTION, LOSS OF BUSINESS INFORMATION, OR ANY OTHER PECUNIARY LOSS)
// ARISING OUT OF THE USE OF OR INABILITY TO USE THIS SOFTWARE, EVEN IF NVIDIA HAS
// BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES 

#pragma once
/** @file */

//#include "nvsgcommon.h"

#include  <math.h>
#include  <algorithm> // for std::max, std::min (_MAX, _MIN)
#include  <float.h>
#include  <limits>

//! Namespace for mathematical helper functions and classes.
namespace nvmath
{
  // define the stand trig values
#ifdef PI
# undef PI
# undef PI_HALF
# undef PI_QUARTER
#endif
  //! constant PI
  const float PI            = 3.14159265358979323846f;
  //! constant PI half
  const float PI_HALF       = 1.57079632679489661923f;
  //! constant PI quarter
  const float PI_QUARTER    = 0.78539816339744830962f;
  //! constant square root two
  const float SQRT_TWO      = 1.41421356237309504880f;
  //! constant square root two half
  const float SQRT_TWO_HALF = 0.70710678118654752440f;

  //! Template to clamp an object of type T to a lower and an upper limit.
  /** \returns clamped value of \a v between \a l and \a u */
  template<class T> T     clamp( T v                  //!<  value to clamp
                               , T l                  //!<  lower limit
                               , T u                  //!<  upper limit
                               )
  {
    return std::min(u, std::max(l,v));
  }

  //! Transform an angle in degrees to radians.
  /** \returns angle in radians */
  template<typename T>
  inline T degToRad( T angle )
  {
    return( angle*(PI/180) );
  }

  /*! \brief Returns the highest bit set for the specified input value */
  template<typename T>
  inline int highestBit( T i )
  {
    //NVSG_CTASSERT( std::numeric_limits<T>::is_integer );
    int hb = -1;
    for ( T h = i ; h ; h>>=1, hb++ )
      ;
    return hb;
  }

  /*! \brief Returns the highest bit value for the specified input value */
  template<typename T>
  inline T highestBitValue(T i)
  {
    //NVSG_CTASSERT( std::numeric_limits<T>::is_integer );
    T h = 0;
    while (i)
    {
      h = (i & (~i+1)); // grab lowest bit
      i &= ~h; // clear lowest bit
    }
    return h;
  }

  /*! \brief Calculate the vertical field of view out of the horizontal field of view */
  inline float horizontalToVerticalFieldOfView( float hFoV, float aspectRatio )
  {
    return( 2.0f * atanf( tanf( 0.5f * hFoV ) / aspectRatio ) );
  }

  //! Determine if an integer is a power of two.
  /** \returns \c true if \a n is a power of two, otherwise \c false  */
  template<typename T>
  inline bool isPowerOfTwo( T n )
  {
    //NVSG_CTASSERT( std::numeric_limits<T>::is_integer );
    return (n && !(n & (n-1)));
  }

  //!  Linear interpolation between two values \a v0 and \a v1.
  /**  v = ( 1 - alpha ) * v0 + alpha * v1 */
  template<typename T> 
  inline T lerp( float alpha  //!<  interpolation parameter
               , const T &v0  //!<  starting value
               , const T &v1  //!<  ending value
               )
  {
    return( (T)(v0 + alpha * ( v1 - v0 )) );
  }

  template<typename T> 
  inline T lerp( double alpha  //!<  interpolation parameter
               , const T &v0  //!<  starting value
               , const T &v1  //!<  ending value
               )
  {
    return( (T)(v0 + alpha * ( v1 - v0 )) );
  }

  template<typename T>
  inline void lerp( float alpha, const T & v0, const T & v1, T & vr )
  {
    vr = v0 + alpha * ( v1 - v0 );
  }

  /*! \brief Determine the maximal value out of three.
   *  \param a A constant reference of the first value to consider.
   *  \param b A constant reference of the second value to consider.
   *  \param c A constant reference of the third value to consider.
   *  \return A constant reference to the maximal value out of \a a, \a b, and \a c.
   *  \sa min */
  template<typename T>  
  inline const T & nv_max( const T &a, const T &b, const T &c )
  {
    return( std::max( a, std::max( b, c ) ) );
  }

  /*! \brief Determine the minimal value out of three.
   *  \param a A constant reference of the first value to consider.
   *  \param b A constant reference of the second value to consider.
   *  \param c A constant reference of the third value to consider.
   *  \return A constant reference to the minimal value out of \a a, \a b, and \a c.
   *  \sa max */
  template<typename T>  
  inline const T & nv_min( const T &a, const T &b, const T &c )
  {
    return( std::min( a, std::min( b, c ) ) );
  }

  /*! \brief Determine the smallest power of two equal to or larger than \a n.
   *  \param n The value to determine the nearest power of two above.
   *  \return \a n, if \a n is a power of two, otherwise the smallest power of two above \a n.
   *  \sa powerOfTowBelow */
  template<typename T>
  inline T powerOfTwoAbove( T n )
  {
    //NVSG_CTASSERT( std::numeric_limits<T>::is_integer );
    if ( isPowerOfTwo( n ) )
    {
      return( n );
    }
    else
    {
      return highestBitValue(n) << 1;
    }
  }

  /*! \brief Determine the largest power of two equal to or smaller than \a n.
   *  \param n The value to determine the nearest power of two below.
   *  \return \a n, if \a n is a power of two, otherwise the largest power of two below \a n.
   *  \sa powerOfTowAbove */
  template<typename T>
  inline T powerOfTwoBelow( T n )
  {
    //NVSG_CTASSERT( std::numeric_limits<T>::is_integer );
    if ( isPowerOfTwo( n ) )
    {
      return( n );
    }
    else
    {
      return highestBitValue(n);
    }
  }

  /*! \brief Calculates the nearest power of two for the specified integer. 
   *  \param n Integer for which to calculate the nearest power of two. 
   *  \returns nearest power of two for integer \a n. */ 
  template<typename T>
  inline T powerOfTwoNearest( T n)
  {
    //NVSG_CTASSERT( std::numeric_limits<T>::is_integer );
    if ( isPowerOfTwo( n ) )
    {
      return n;
    }
    
    T prev = highestBitValue(n); // POT below
    T next = prev<<1; // POT above
    return (next-n) < (n-prev) ? next : prev; // return nearest
  }

  //! Transform an angle in radian to degree.
  /** \param angle angle in radians
    * \returns angle in degrees */
  inline float radToDeg( float angle )
  {
    return( angle*180.0f/PI );
  }

  //! Determine the sign of a scalar.
  /** \param t scalar value
    * \returns sign of \a t */
  template<typename T> 
  inline int sign( const T &t )
  {
    return( ( t < 0 ) ? -1 : ( ( t > 0 ) ? 1 : 0 ) );
  }

  //! Template to square an object of Type T.
  /** \param t value to square
    * \returns product of \a t with itself */
  template<typename T> 
  inline T square( const T& t )
  {
    return( t * t );
  }

  /*! \brief Calculate the horizontal field of view out of the vertical field of view */
  inline float verticalToHorizontalFieldOfView( float vFoV, float aspectRatio )
  {
    return( 2.0f * atanf( tanf( 0.5f * vFoV ) * aspectRatio ) );
  }

  //! Compares two numerical values
  /** \returns -1 if \a lhs < \a rhs, 1 if \a lhs > \a rhs, and 0 if \a lhs == \a rhs. */
  template<typename T> 
  inline int compare(T lhs, T rhs)
  {
    return (lhs == rhs) ? 0 : (lhs < rhs) ? -1 : 1;
  }

  // specializations for float and double below

  //! Compares two float values
  /** \returns -1 if \a lhs < \a rhs, 1 if \a lhs > \a rhs, and 0 if \a lhs == \a rhs. */
  template<> 
  inline int compare(float lhs, float rhs)
  {
    return ((fabs(lhs-rhs) < FLT_EPSILON) ? 0 : (lhs < rhs) ? -1 : 1);
  }

  //! Compares two double values
  /** \returns -1 if \a lhs < \a rhs, 1 if \a lhs > \a rhs, and 0 if \a lhs == \a rhs. */
  template<> 
  inline int compare(double lhs, double rhs)
  {
    return ((fabs(lhs-rhs) < DBL_EPSILON) ? 0 : (lhs < rhs) ? -1 : 1);
  }
}

#endif
