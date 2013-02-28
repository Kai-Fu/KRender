#ifndef _nvsg_nvmath_Spherent_
#define _nvsg_nvmath_Spherent_

// Copyright NVIDIA Corporation 2002-2005
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

#include "Vecnt.h"

namespace nvmath
{
  /*! \brief Sphere class.
   *  \remarks This is a sphere in n-space, determined by a center point and a radius. A sphere is
   *  often used as a bounding sphere around some data, so there is a variety of routines to
   *  calculate the bounding sphere. */
  template<unsigned int n, typename T> class Spherent
  {
    public:
      /*! \brief Default constructor.
       *  \remarks The default sphere is initialized to be invalid. A sphere is considered to be
       *  invalid, if the radius is negative. */
      Spherent();

      /*! \brief Constructor by center and radius.
       *  \param center The center of the sphere.
       *  \param radius The radius of the sphere. */
      Spherent( const Vecnt<n,T> &center, T radius );

    public:
      /*! \brief Get the center of the sphere.
       *  \return The center of the sphere.
       *  \sa setCenter, getRadius */
      const Vecnt<n,T> & getCenter() const;

      /*! \brief Get the radius of the sphere.
       *  \return The radius of the sphere.
       *  \remarks A sphere is considered to be invalid if the radius is negative.
       *  \sa setRadius, getCenter, invalidate */
      T getRadius() const;

      /*! \brief Invalidate the sphere by setting the radius to a negative value.
       *  \remarks A sphere is considered to be invalid if the radius is negative.
       *  \sa getRadius, setRadius */
      void invalidate();

      /*! \brief Function call operator.
       *  \param p A reference to a constant point.
       *  \return The signed distance of the point \a p from the sphere.
       *  \remarks The distance is negative when \a p is inside the sphere, and it is positive if \a
       *  p is outside the sphere. */
      T operator()( const Vecnt<n,T> &p ) const;

      /*! \brief Set the center of the sphere.
       *  \param center A reference to the constant center point.
       *  \sa getCenter, setRadius */
      void setCenter( const Vecnt<n,T> &center );

      /*! \brief Set the radius of the sphere.
       *  \param radius The radius of the sphere.
       *  \sa getRadius, setRadius, invalidate */
      void setRadius( T radius );

    private:
      Vecnt<n,T>  m_center;
      T           m_radius;
  };

  // - - - - -  - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
  // non-member functions
  // - - - - -  - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

  /*! \brief Determine the bounding sphere of a number of points.
   *  \param points A pointer to the points to include.
   *  \param numberOfPoints The number of points used.
   *  \return A small sphere around the given points.
   *  \remarks The sphere is not necessarily the smallest possible bounding sphere. */
  template<unsigned int n, typename T>
    Spherent<n,T> boundingSphere( const Vecnt<n,T> * points, unsigned int numberOfPoints );

  /*! \brief Determine the bounding sphere of a number of indexed points.
   *  \param points A pointer to the points to use.
   *  \param indices A pointer to the indices to use.
   *  \param numberOfIndices The number of indices used.
   *  \return A small sphere around the given points.
   *  \remarks The sphere is not necessarily the smallest possible bounding sphere. */
  template<unsigned int n, typename T>
    Spherent<n,T> boundingSphere( const Vecnt<n,T> * points, const unsigned int * indices
                                , unsigned int numberOfIndices );

  /*! \brief Determine the bounding sphere of a number of strip-indexed points.
   *  \param points A pointer to the points to use.
   *  \param strips A pointer to the strips of indices used.
   *  \param numberOfStrips The number of strips used.
   *  \return A small sphere around the given points.
   *  \remarks The sphere is not necessarily the smallest possible bounding sphere. */
  template<unsigned int n, typename T>
    Spherent<n,T> boundingSphere( const Vecnt<n,T> * points
                                , const std::vector<unsigned int> * strips
                                , unsigned int numberOfStrips );

  /*! \brief Determine the bounding sphere of a sphere and a point.
   *  \param s A reference to the constant sphere to use.
   *  \param p A reference to the constant point to use.
   *  \return The bounding sphere around \a s and \a p.
   *  \remarks If \a p is inside of \a s, a sphere of the same size and position as \a s is
   *  returned. Otherwise, the smallest sphere enclosing both \a s and \a p is returned. */
  template<unsigned int n, typename T>
    Spherent<n,T> boundingSphere( const Spherent<n,T> & s, const Vecnt<n,T> & p );

  /*! \brief Determine the bounding sphere around two spheres.
   *  \param s0 A reference to the constant first sphere to enclose.
   *  \param s1 A reference to the constant second sphere to enclose.
   *  \return A bounding sphere around \a s0 and \a s1.
   *  \remarks If the one sphere is completely contained in the other, a sphere equaling that other
   *  sphere is returned. Otherwise, the smallest sphere enclosing both \a s0 and \a s1 is returned. */
  template<unsigned int n, typename T>
    Spherent<n,T> boundingSphere( const Spherent<n,T> & s0, const Spherent<n,T> &s1 );

  /*! \brief Determine the bounding sphere of the intersection of two spheres.
   *  \param s0 A reference to the constant first sphere.
   *  \param s1 A reference to the constant second sphere.
   *  \return A sphere around the intersection of \a s0 and \a s1. */
  template<unsigned int n, typename T>
    Spherent<n,T> intersectingSphere( const Spherent<n,T> & s0, const Spherent<n,T> & s1 );

  /*! \brief Determine if a sphere is empty.
   *  \param s A reference to a constant sphere.
   *  \return \c true, if \ s is empty, otherwise \c false.
   *  \remarks A sphere is considered to be empty, if it's radius is negative. */
  template<unsigned int n, typename T>
    bool isEmpty( const Spherent<n,T> & s );

  /*! \brief Test for equality of two spheres.
   *  \param s0 A constant reference to the first sphere to test.
   *  \param s1 A constant reference to the second sphere to test.
   *  \return \c true, if the centers of the spheres are equal and the radii differ less than the
   *  type specific epsilon. Otherwise \c false.
   *  \sa operator!=() */
  template<unsigned int n, typename T>
    bool operator==( const Spherent<n,T> & s0, const Spherent<n,T> & s1 );

  /*! \brief Test for inequality of two spheres.
   *  \param s0 A constant reference to the first sphere to test.
   *  \param s1 A constant reference to the second sphere to test.
   *  \return \c true, if the centers of the spheres are different or the radii differ more than
   *  the type specific epsilon. Otherwise \c false.
   *  \sa operator==() */
  template<unsigned int n, typename T>
  bool operator!=( const Spherent<n,T> & s0, const Spherent<n,T> & s1 );

  // - - - - -  - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
  // Convenience type definitions
  // - - - - -  - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
  typedef Spherent<3,float>   Sphere3f;
  typedef Spherent<3,double>  Sphere3d;

  // - - - - -  - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
  // inlined member functions
  // - - - - -  - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
  template<unsigned int n, typename T>
  inline Spherent<n,T>::Spherent()
    : m_center(0.0f,0.0f,0.0f)
    , m_radius(-1.0f)
  {
  }

  template<unsigned int n, typename T>
  inline Spherent<n,T>::Spherent( const Vecnt<n,T> &center, T radius )
    : m_center(center)
    , m_radius(radius)
  {
  }

  template<unsigned int n, typename T>
  inline const Vecnt<n,T> & Spherent<n,T>::getCenter() const
  {
    return( m_center );
  }

  template<unsigned int n, typename T>
  inline T Spherent<n,T>::getRadius() const
  {
    return( m_radius );
  }

  template<unsigned int n, typename T>
  inline void Spherent<n,T>::invalidate()
  {
    m_radius = T(-1);
  }

  template<unsigned int n, typename T>
  inline T Spherent<n,T>::operator()( const Vecnt<n,T> &p ) const
  {
    return( distance( m_center, p ) - m_radius );
  }

  template<unsigned int n, typename T>
  inline void Spherent<n,T>::setCenter( const Vecnt<n,T> &center )
  {
    m_center = center;
  }

  template<unsigned int n, typename T>
  inline void Spherent<n,T>::setRadius( T radius )
  {
    m_radius = radius;
  }

  template<unsigned int n, typename T>
  inline Spherent<n,T> boundingSphere( const Vecnt<n,T> * points, unsigned int numberOfPoints )
  {
    //NVSG_ASSERT( points );
    //  determine the bounding box
    Vecnt<n,T> bbox[2];
    bbox[0] = points[0];
    bbox[1] = points[0];
    for ( unsigned int i=1 ; i<numberOfPoints ; i++ )
    {
      for ( unsigned int j=0 ; j<n ; j++ )
      {
        if ( points[i][j] < bbox[0][j] )
        {
          bbox[0][j] = points[i][j];
        }
        else if ( bbox[1][j] < points[i][j] )
        {
          bbox[1][j] = points[i][j];
        }
      }
    }

    //  take the center of the bounding box as the center of the bounding sphere
    Vecnt<n,T> center = T(0.5) * ( bbox[0] + bbox[1] );

    //  and determine the minimal radius needed from that center points
    T minRadius(0);
    for ( unsigned int i=0 ; i<numberOfPoints ; i++ )
    {
      T d = lengthSquared( points[i] - center );
      if ( minRadius < d )
      {
        minRadius = d;
      }
    }
    return( Spherent<n,T>( center, sqrt( minRadius ) ) );
  }

  template<unsigned int n, typename T>
  Spherent<n,T> boundingSphere( const Vecnt<n,T> * points, const unsigned int * indices
                              , unsigned int numberOfIndices )
  {
    //NVSG_ASSERT( points && indices );
    //  determine the bounding box
    Vecnt<n,T> bbox[2];
    bbox[0] = points[indices[0]];
    bbox[1] = points[indices[0]];
    for ( unsigned int i=1 ; i<numberOfIndices ; i++ )
    {
      unsigned int idx = indices[i];
      for ( unsigned int j=0 ; j<n ; j++ )
      {
        if ( points[idx][j] < bbox[0][j] )
        {
          bbox[0][j] = points[idx][j];
        }
        else if ( bbox[1][j] < points[idx][j] )
        {
          bbox[1][j] = points[idx][j];
        }
      }
    }

    //  take the center of the bounding box as the center of the bounding sphere
    Vecnt<n,T> center = T(0.5) * ( bbox[0] + bbox[1] );

    //  and determine the minimal radius needed from that center points
    T minRadius(0);
    for ( unsigned int i=0 ; i<numberOfIndices ; i++ )
    {
      T d = lengthSquared( points[indices[i]] - center );
      if ( minRadius < d )
      {
        minRadius = d;
      }
    }
    return( Spherent<n,T>( center, sqrt( minRadius ) ) );
  }

  template<unsigned int n, typename T>
  Spherent<n,T> boundingSphere( const Vecnt<n,T> * points
                              , const std::vector<unsigned int> * strips
                              , unsigned int numberOfStrips )
  {
    //NVSG_ASSERT( points && strips );
    //  determine the bounding box
    Vecnt<n,T> bbox[2];
    bbox[0] = points[strips[0][0]];
    bbox[1] = points[strips[0][0]];
    for ( unsigned int i=0 ; i<numberOfStrips ; i++ )
    {
      for ( size_t j=0 ; j<strips[i].size() ; j++ )
      {
        unsigned int idx = strips[i][j];
        for ( unsigned int k=0 ; k<n ; k++ )
        {
          if ( points[idx][k] < bbox[0][k] )
          {
            bbox[0][k] = points[idx][k];
          }
          else if ( bbox[1][k] < points[idx][k] )
          {
            bbox[1][k] = points[idx][k];
          }
        }
      }
    }

    //  take the center of the bounding box as the center of the bounding sphere
    Vecnt<n,T> center = T(0.5) * ( bbox[0] + bbox[1] );

    //  and determine the minimal radius needed from that center points
    T minRadius = 0.0f;
    for ( unsigned int i=0 ; i<numberOfStrips ; i++ )
    {
      for ( size_t j=0 ; j<strips[i].size() ; j++ )
      {
        T d = lengthSquared( points[strips[i][j]] - center );
        if ( minRadius < d )
        {
          minRadius = d;
        }
      }
    }
    return( Spherent<n,T>( center, sqrt( minRadius ) ) );
  }

  template<unsigned int n, typename T>
  Spherent<n,T> boundingSphere( const Spherent<n,T> & s, const Vecnt<n,T> & p )
  {
    Vecnt<n,T> center(s.getCenter());
    T radius(s.getRadius());
    if ( ! isEmpty( s ) )
    {
      T dist = distance( center, p );
      if ( radius < dist )
      {
        //  the point is outside the sphere
        //  the new center is a weighted sum of the old and the point
        center = ( ( dist + radius ) * center + ( dist - radius ) * p ) / ( 2 * dist );
        //  and the new radius is half the sum of the distance and the radius
        radius = T(0.5) * ( radius + dist );
      }
    }
    return( Spherent<n,T>( center, radius ) );
  }

  template<unsigned int n, typename T>
  Spherent<n,T> boundingSphere( const Spherent<n,T> & s0, const Spherent<n,T> & s1 )
  {
    Spherent<n,T> s;
    if ( isEmpty( s0 ) )
    {
      s = s1;
    }
    else if ( isEmpty( s1 ) )
    {
      s = s0;
    }
    else
    {
      T dist = distance( s0.getCenter(), s1.getCenter() );
      if ( dist + s1.getRadius() <= s0.getRadius() )
      {
        //  s1 is completely inside of s0, so return s0
        s = s0;
      }
      else if ( dist + s0.getRadius() <= s1.getRadius() )
      {
        //  s0 is completely inside of s1, so return s1
        s = s1;
      }
      else
      {
        //  the spheres don't include each other
        if ( dist < std::numeric_limits<T>::epsilon() )
        {
          //  the centers are too close together to calculate the weighted sum, so just take the
          //  center of them (knowing, that the radii also have to be close together; see above)
          s.setCenter( 0.5f * ( s0.getCenter() + s1.getCenter() ) );
          s.setRadius( 0.5f * dist + std::max( s0.getRadius(), s1.getRadius() ) );
        }
        else
        {
          //  the new center is a weighted sum of the two given
          s.setCenter( (  ( dist + s0.getRadius() - s1.getRadius() ) * s0.getCenter()
                        + ( dist + s1.getRadius() - s0.getRadius() ) * s1.getCenter() ) / ( 2 * dist ) );
          //  and the new center is half the sum of the distance and the radii
          s.setRadius( T(0.5) * ( s0.getRadius() + dist + s1.getRadius() ) );
        }
      }
    }
    return( s );
  }

  template<unsigned int n, typename T>
  Spherent<n,T> intersectingSphere( const Spherent<n,T> & s0, const Spherent<n,T> & s1 )
  {
    Vecnt<n,T> v = s1.getCenter() - s0.getCenter();
    T d = v.normalize();
    Spherent<n,T> s;   //  the default constructor of a Spherent<n,T> creates an empty sphere
    if ( d < ( s0.getRadius() + s1.getRadius() ) )
    {
      //  the spheres do intersect
      T ds = d * d;
      T r0s = square( s0.getRadius() );
      T r1s = square( s1.getRadius() );
      if ( ( ds + r0s ) <= r1s )
      {
        //  s0 is inside of s1 => intersection is s0
        s = s0;
      }
      else if ( ( ds + r1s ) <= r0s )
      {
        //  s1 is inside of s0 => intersection is s1
        s = s1;
      }
      else
      {
        //  nontrivial intersection here
        T e = ( ds + r0s - r1s ) / ( 2 * d );
        s.setRadius( sqrt( r0s - e*e ) );
        s.setCenter( s0.getCenter() + e * v );
      }
    }
    return( s );
  }

  template<unsigned int n, typename T>
  inline bool isEmpty( const Spherent<n,T> & s )
  {
    return( s.getRadius() < 0 );
  }

  template<unsigned int n, typename T>
  bool operator==( const Spherent<n,T> & s0, const Spherent<n,T> & s1 )
  {
    return(   ( s0.getCenter() == s1.getCenter() )
          &&  abs( s0.getRadius() - s1.getRadius() ) < std::numeric_limits<T>::epsilon() );
  }

  template<unsigned int n, typename T>
  bool operator!=( const Spherent<n,T> & s0, const Spherent<n,T> & s1 )
  {
    return( ! ( s0 == s1 ) );
  }

  //! global unit sphere.
//  extern const Sphere3f cUnitSphere;

} // namespace nvmath

#endif