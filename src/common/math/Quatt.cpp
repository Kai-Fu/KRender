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

#include "Quatt.h"

namespace nvmath
{
  template<typename T>
  Quatt<T> _lerp( T alpha, const Quatt<T> & q0, const Quatt<T> & q1 )
  {
    //  cosine theta = dot product of q0 and q1
    T cosTheta = q0[0] * q1[0] + q0[1] * q1[1] + q0[2] * q1[2] + q0[3] * q1[3];

    //  if q1 is on opposite hemisphere from q0, use -q1 instead
    bool flip = ( cosTheta < 0 );
    if ( flip )
    {
      cosTheta = - cosTheta;
    }

    T beta;
    if ( 1 - cosTheta < std::numeric_limits<T>::epsilon() )
    {
      //  if q1 is (within precision limits) the same as q0, just linear interpolate between q0 and q1.
      beta = 1 - alpha;
    }
    else
    {
      //  normal case
      T theta = acos( cosTheta );
      T oneOverSinTheta = 1 / sin( theta );
      beta = sin( theta * ( 1 - alpha ) ) * oneOverSinTheta;
      alpha = sin( theta * alpha ) * oneOverSinTheta;
    }

    if ( flip )
    {
      alpha = - alpha;
    }

    return( Quatt<T>( beta * q0[0] + alpha * q1[0]
                    , beta * q0[1] + alpha * q1[1]
                    , beta * q0[2] + alpha * q1[2]
                    , beta * q0[3] + alpha * q1[3] ) );
  }

  Quatt<float> lerp( float alpha, const Quatt<float> & q0, const Quatt<float> & q1 )
  {
    return( _lerp( alpha, q0, q1 ) );
  }

}

