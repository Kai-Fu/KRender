#ifndef _nvsg_nvmath_Planent_
#define _nvsg_nvmath_Planent_

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

#include "nvsgcommon.h"
#include "nvmath/Vecnt.h"

#include <float.h>

namespace nvmath
{
  /*! \brief Plane class
   *  \remarks A Plane is a Hyperplane in n-space. It's defined by a normal and an offset, such that
   *  for any point x in the plane, the equation \code normal * x + offset = 0 \endcode holds. */
  template<unsigned int n,typename T> class Planent
  {
    public:
      /*! \brief Default constructor.
       *  \remarks For performance reasons no initialization is performed. */
      Planent();

      /*! \brief Constructor by normal and an offset.
       *  \param normal The normal of the plane.
       *  \param offset The offset of the plane.
       *  \note It is assumed that \a normal is normalized. */
      Planent( const Vecnt<n,T> & normal, T offset );

      /*! \brief Constructor by normal and a point on the plane.
       *  \param normal The normal of the plane.
       *  \param p An arbitrary point on the plane.
       *  \remarks The plane containing the point \a p, with the plane normal \a normal is created. */
      Planent( const Vecnt<n,T> & normal, const Vecnt<n,T> & p );

      /*! \brief Constructor by three points on the plane.
       *  \param p0 A first point on the plane.
       *  \param p1 A second point on the plane.
       *  \param p2 A third point on the plane.
       *  \remarks Three points uniquely define a plane. */
      Planent( const Vecnt<n,T> & p0, const Vecnt<n,T> & p1, const Vecnt<n,T> & p2 );

    public:
      /*! \brief Get the normal of the plane.
       *  \return A constant reference to the normal of the plane.
       *  \sa getOffset, setNormal */
      const Vecnt<n,T> & getNormal() const;

      /*! \brief Get the offset of the plane.
       *  \return The offset of the plane.
       *  \sa getNormal, setOffset */
      T getOffset() const;

      /*! \brief Function call operator.
       *  \param p A reference to a constant point.
       *  \return The signed distance of the point \a p from the plane. */
      T operator()( const Vecnt<n,T> & p ) const;

      /*! \brief Set the normal of the plane.
       *  \param normal A reference to the constant normal to use.
       *  \sa getNormal, setOffset */
      void setNormal( const Vecnt<n,T> & normal );

      /*! \brief Set the offset of the plane.
       *  \param offset The offset to use.
       *  \sa getOffset, setNormal */
      void setOffset( T offset );

    private:
      Vecnt<n,T>  m_normal;
      T           m_offset;
  };

  // - - - - -  - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
  // non-member functions
  // - - - - -  - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

  /*! \brief Test if two points are on opposite sides of a plane.
   *  \param pl A reference to a constant plane.
   *  \param p0 A reference to the constant first point.
   *  \param p1 A reference to the constant second point.
   *  \return \c true, if \a p0 and \a p1 are on opposite sides of \a pl.
   *  \sa areOnSameSide */
  template<unsigned int n, typename T>
    bool areOnOppositeSides( const Planent<n,T> & pl, const Vecnt<n,T> & p0, const Vecnt<n,T> & p1 );

  /*! \brief Test if two points are on the same side of a plane.
   *  \param pl A reference to a constant plane.
   *  \param p0 A reference to the constant first point.
   *  \param p1 A reference to the constant second point.
   *  \return \c true, if \a p0 and \a p1 are on the same side of \a pl.
   *  \sa areOnOppositeSide */
  template<unsigned int n, typename T>
    bool areOnSameSide( const Planent<n,T> & pl, const Vecnt<n,T> & p0, const Vecnt<n,T> & p1 );

  /*! \brief Calculate the distance of a point to a plane.
   *  \param pl A reference to the constant plane.
   *  \param p A reference to the constant point.
   *  \return The distance of the point \a p to the plane \a pl.
   *  \sa signedDistance */
  template<unsigned int n, typename T>
    T distance( const Planent<n,T> & pl, const Vecnt<n,T> & p );

  /*! \brief Calculate the point on a plane nearest to a give point.
   *  \param pl A reference to the constant plane.
   *  \param p A reference to the constant point.
   *  \return The point on the plane \a pl nearest to the point \a p.
   *  \sa distance, signedDistance */
  template<unsigned int n, typename T>
    Vecnt<n,T> nearestPoint( const Planent<n,T> & pl, const Vecnt<n,T> & p );

  /*! \brief Test for equality of two planes.
   *  \param pl0 A reference to the constant first plane.
   *  \param pl1 A reference to the constant second plane.
   *  \return \c true, if the normals of the two planes are equal and the offsets differ less than
   *  the type specific epsilon. Otherwise \c false.
   *  \sa operator!=() */
  template<unsigned int n, typename T>
    bool operator==( const Planent<n,T> & pl0, const Planent<n,T> & pl1 );

  /*! \brief Test for inequality of two planes.
   *  \param pl0 A reference to the constant first plane.
   *  \param pl1 A reference to the constant second plane.
   *  \return \c true, if the normals of the two planes are not equal or the offsets differ more
   *  than the type specific epsilon. Otherwise \c false.
   *  \sa operator==() */
  template<unsigned int n, typename T>
    bool operator!=( const Planent<n,T> & pl0, const Planent<n,T> & pl1 );

  /*! \brief Calculate the signed distance of a point to a plane.
   *  \param pl A reference to the constant plane.
   *  \param p A reference to the constant point.
   *  \return The signed distance of the point \a p to the plane \a pl.
   *  \remarks The signed distance of a point \a p to a plane \a pl is positive, if \a p is in the
   *  half space the normal a \a pl points to. If \a p is in the other half space, the signed
   *  distance is negative.
   *  \sa distance */
  template<unsigned int n, typename T>
    T signedDistance( const Planent<n,T> & pl, const Vecnt<n,T> & p );

  // - - - - -  - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
  // Convenience type definitions
  // - - - - -  - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
  typedef Planent<3,float>  Plane3f;
  typedef Planent<3,double> Plane3d;

  // - - - - -  - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
  // inlined member functions
  // - - - - -  - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
  template<unsigned int n, typename T>
  inline Planent<n,T>::Planent()
  {
  }

  template<unsigned int n, typename T>
  inline Planent<n,T>::Planent( const Vecnt<n,T> & normal, T offset )
    : m_normal(normal)
    , m_offset(offset)
  {
    //NVSG_PRIVATE_ASSERT( isNormalized( m_normal ) );
  }

  template<unsigned int n, typename T>
  inline Planent<n,T>::Planent( const Vecnt<n,T> & normal, const Vecnt<n,T> & p )
    : m_normal(normal)
    , m_offset(-p*normal)
  {
    //NVSG_PRIVATE_ASSERT( isNormalized( m_normal ) );
  }

  template<unsigned int n, typename T>
  inline Planent<n,T>::Planent( const Vecnt<n,T> & p0, const Vecnt<n,T> & p1, const Vecnt<n,T> & p2 )
  {
    //NVSG_PRIVATE_ASSERT( ( p0 != p1 ) && ( p0 != p2 ) && ( p1 != p2 ) );
    m_normal = ( p1 - p0 ) ^ ( p2 - p0 );
    m_normal.normalize();
    m_offset = - m_normal * p0;
  }

  template<unsigned int n, typename T>
  inline const Vecnt<n,T> & Planent<n,T>::getNormal() const
  {
    return( m_normal );
  }

  template<unsigned int n, typename T>
  inline T Planent<n,T>::getOffset() const
  {
    return( m_offset );
  }

  template<unsigned int n, typename T>
  inline T Planent<n,T>::operator()( const Vecnt<n,T> & p ) const
  {
    return( m_normal * p + m_offset );
  }

  template<unsigned int n, typename T>
  inline void Planent<n,T>::setNormal( const Vecnt<n,T> & normal )
  {
    //NVSG_PRIVATE_ASSERT( isNormalized( normal ) );
    m_normal = normal;
  }

  template<unsigned int n, typename T>
  inline void Planent<n,T>::setOffset( T offset )
  {
    m_offset = offset;
  }

  // - - - - -  - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
  // inlined non-member functions
  // - - - - -  - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

  template<unsigned int n, typename T>
  bool areOnOppositeSides( const Planent<n,T> & pl, const Vecnt<n,T> & p0, const Vecnt<n,T> & p1 )
  {
    return( pl( p0 ) * pl( p1 ) < 0 );
  }

  template<unsigned int n, typename T>
  bool areOnSameSide( const Planent<n,T> & pl, const Vecnt<n,T> & p0, const Vecnt<n,T> & p1 )
  {
    return( 0 < pl( p0 ) * pl( p1 ) );
  }

  template<unsigned int n, typename T>
  T distance( const Planent<n,T> & pl, const Vecnt<n,T> & p )
  {
    return( abs( pl( p ) ) );
  }

  template<unsigned int n, typename T>
  Vecnt<n,T> nearestPoint( const Planent<n,T> & pl, const Vecnt<n,T> & p )
  {
    return( p - pl( p ) * pl.getNormal() );
  }

  template<unsigned int n, typename T>
  bool operator==( const Planent<n,T> & pl0, const Planent<n,T> & pl1 )
  {
    return(   ( pl0.getNormal() == pl1.getNormal() )
          &&  ( abs( pl0.getOffset() - pl1.getOffset() ) < FLT_EPSILON ) );
  }

  template<unsigned int n, typename T>
  bool operator!=( const Planent<n,T> & pl0, const Planent<n,T> & pl1 )
  {
    return( ! ( pl0 == pl1 ) );
  }

  template<unsigned int n, typename T>
  T signedDistance( const Planent<n,T> & pl, const Vecnt<n,T> & p )
  {
    return( pl( p ) );
  }

} // end namespace nvmath

#endif
