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


#include "nvmath.h"
#include "Trafo.h"

using namespace nvmath;

Trafo::Trafo( void )
: m_center( Vec3f( 0.0f, 0.0f, 0.0f ) )
, m_orientation( Quatf( 0.0f, 0.0f, 0.0f, 1.0f ) )
, m_scaleOrientation( Quatf( 0.0f, 0.0f, 0.0f, 1.0f ) )
, m_scaling( Vec3f( 1.0f, 1.0f, 1.0f ) )
, m_translation( Vec3f( 0.0f, 0.0f, 0.0f ) )
{
  //NVSG_TRACE();
};

Trafo::Trafo( const Trafo &rhs )
: m_center( rhs.m_center )
, m_orientation( rhs.m_orientation )
, m_scaleOrientation( rhs.m_scaleOrientation )
, m_scaling( rhs.m_scaling )
, m_translation( rhs.m_translation )
{
  //NVSG_TRACE();
}

Trafo & Trafo::operator=( const Trafo & rhs )
{
  //NVSG_TRACE();
  if (&rhs != this)
  {
    m_center            = rhs.m_center;
    m_orientation       = rhs.m_orientation;
    m_scaleOrientation  = rhs.m_scaleOrientation;
    m_scaling           = rhs.m_scaling;
    m_translation       = rhs.m_translation;
  }
  return *this;
}

Mat44f Trafo::getMatrix( void ) const
{
  //NVSG_TRACE();
  //  Calculates -C * SO^-1 * S * SO * R * C * T, with
  //    C     being the center translation
  //    SO    being the scale orientation
  //    S     being the scale
  //    R     being the rotation
  //    T     being the translation
  Mat33f soInv( -m_scaleOrientation );
  Mat44f m0( (Vec4f( soInv[0], 0.0f ))
           , (Vec4f( soInv[1], 0.0f ))
           , (Vec4f( soInv[2], 0.0f ))
           , (Vec4f( -m_center*soInv, 1.0f )) );
  Mat33f rot( m_scaleOrientation * m_orientation );
  Mat44f m1( (Vec4f( m_scaling[0] * rot[0], 0.0f ))
           , (Vec4f( m_scaling[1] * rot[1], 0.0f ))
           , (Vec4f( m_scaling[2] * rot[2], 0.0f ))
           , (Vec4f( m_center + m_translation, 1.0f )) );
  return( m0 * m1 );
}

Mat44f Trafo::getInverse( void ) const
{
  //NVSG_TRACE();
  //  Calculates T^-1 * C^-1 * R^-1 * SO^-1 * S^-1 * SO * -C^-1, with
  //    C     being the center translation
  //    SO    being the scale orientation
  //    S     being the scale
  //    R     being the rotation
  //    T     being the translation
  Mat33f rot( -m_orientation * -m_scaleOrientation );
  Mat44f m0( (Vec4f( rot[0], 0.0f )),
             (Vec4f( rot[1], 0.0f )),
             (Vec4f( rot[2], 0.0f )),
             (Vec4f( ( - m_center - m_translation ) * rot, 1.0f )) );
  Mat33f so( m_scaleOrientation );

  Vec3f scale(m_scaling);
  if( (fabs(m_scaling[0]) < std::numeric_limits<float>::epsilon()) ||
	  (fabs(m_scaling[1]) < std::numeric_limits<float>::epsilon()) ||
	  (fabs(m_scaling[2]) < std::numeric_limits<float>::epsilon()) )
  {
	  scale[0] = 1.0f;
	  scale[1] = 1.0f;
	  scale[2] = 1.0f;
  }
  Mat44f m1( (Vec4f( so[0], 0.0f ) / scale[0]),
             (Vec4f( so[1], 0.0f ) / scale[1]),
             (Vec4f( so[2], 0.0f ) / scale[2]),
             (Vec4f( m_center, 1.0f )) );

  return( m0 * m1 );
}

void Trafo::setIdentity( void )
{
  //NVSG_TRACE();
  m_center = Vec3f( 0.0f, 0.0f, 0.0f );
  m_orientation = Quatf( 0.0f, 0.0f, 0.0f, 1.0f );
  m_scaling = Vec3f( 1.0f, 1.0f, 1.0f );
  m_scaleOrientation = Quatf( 0.0f, 0.0f, 0.0f, 1.0f );
  m_translation = Vec3f( 0.0f, 0.0f, 0.0f );
}

void Trafo::setMatrix( const Mat44f &matrix )
{
  Vec3f translation, scaling;
  Quatf orientation, scaleOrientation;

  decompose( matrix, translation, orientation, scaling, scaleOrientation );

  setCenter( Vec3f( 0.0f, 0.0f, 0.0f ) );
  setTranslation( translation );
  setOrientation( orientation );
  setScaling( scaling );
  setScaleOrientation( scaleOrientation );
}

bool Trafo::operator==( const Trafo &t ) const
{
  //NVSG_TRACE();
  return(   ( getCenter()           == t.getCenter()            )
        &&  ( getOrientation()      == t.getOrientation()       )
        &&  ( getScaling()          == t.getScaling()           )
        &&  ( getScaleOrientation() == t.getScaleOrientation()  )
        &&  ( getTranslation()      == t.getTranslation()       ) );
}

Trafo nvmath::lerp( float alpha, const Trafo &t0, const Trafo &t1 )
{
  //NVSG_TRACE();
  Trafo t;
  t.setCenter( lerp( alpha, t0.getCenter(), t1.getCenter() ) );
  t.setOrientation( lerp( alpha, t0.getOrientation(), t1.getOrientation() ) );
  t.setScaling( lerp( alpha, t0.getScaling(), t1.getScaling() ) );
  t.setScaleOrientation( lerp( alpha, t0.getScaleOrientation(), t1.getScaleOrientation() ) );
  t.setTranslation( lerp( alpha, t0.getTranslation(), t1.getTranslation() ) );
  return( t );
}
