#ifndef _nvsg_nvmath_Matnnt_
#define _nvsg_nvmath_Matnnt_

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

#include "Quatt.h"
#include "Vecnt.h"

namespace nvmath
{
  template<typename T> class Quatt;

  /*! \brief Matrix class of fixed size and type.
   *  \remarks This class is templated by size and type. It holds \a n times \a n values of type \a
   *  T. There are typedefs for the most common usage with 3 and 4 values of type \c float and \c
   *  double: Mat33f, Mat33d, Mat44f, Mat44d. */
  template<unsigned int n, typename T> class Matnnt
  {
    public:
      /*! \brief Default constructor.
       *  \remarks For performance reasons, no initialization is performed. */
      Matnnt();

      /*! \brief Copy constructor from a matrix of possibly different size and type.
       *  \param rhs A matrix with \a m times \a m values of type \a S.
       *  \remarks The minimum \a k of \a n and \a m is determined. The first \a k values of type \a
       *  S in the first \a k rows from \a rhs are converted to type \a T and assigned as the first
       *  \a k values in the first \a k rows of \c this. If \a k is less than \a n, the \a n - \a k
       *  last values of the \a n - \a k last rows of \c this are not initialized. */
      template<unsigned int m, typename S>
        explicit Matnnt( const Matnnt<m,S> & rhs );

      /*! \brief Constructor for a 3 by 3 matrix out of 9 scalar values.
       *  \param m00 First element of the first row of the matrix.
       *  \param m01 Second element of the first row of the matrix.
       *  \param m02 Third element of the first row of the matrix.
       *  \param m10 First element of the second row of the matrix.
       *  \param m11 Second element of the second row of the matrix.
       *  \param m12 Third element of the second row of the matrix.
       *  \param m20 First element of the third row of the matrix.
       *  \param m21 Second element of the third row of the matrix.
       *  \param m22 Third element of the third row of the matrix.
       *  \remarks This constructor can only be used with 3 by 3 matrices, like Mat33f.
       *  \par Example:
       *  \code
       *    Mat33f m33f( 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f );
       *  \endcode */
      Matnnt( T m00, T m01, T m02
            , T m10, T m11, T m12
            , T m20, T m21, T m22 );

      /*! \brief Constructor for a 3 by 3 matrix out of three vectors of three values each.
       *  \param v0 First row of the matrix.
       *  \param v1 Second row of the matrix.
       *  \param v2 Third row of the matrix.
       *  \remarks This constructor can only be used with 3 by 3 matrices, like Mat33f.
       *  \par Example:
       *  \code
       *    Mat33f m33f( xAxis, yAxis, zAxis );
       *  \endcode */
      Matnnt( const Vecnt<n,T> & v0, const Vecnt<n,T> & v1, const Vecnt<n,T> & v2 );

      /*! \brief Constructor for a 4 by 4 matrix out of 16 scalar values.
       *  \param m00 First element of the first row of the matrix.
       *  \param m01 Second element of the first row of the matrix.
       *  \param m02 Third element of the first row of the matrix.
       *  \param m03 Fourth element of the first row of the matrix.
       *  \param m10 First element of the second row of the matrix.
       *  \param m11 Second element of the second row of the matrix.
       *  \param m12 Third element of the second row of the matrix.
       *  \param m13 Fourth element of the second row of the matrix.
       *  \param m20 First element of the third row of the matrix.
       *  \param m21 Second element of the third row of the matrix.
       *  \param m22 Third element of the third row of the matrix.
       *  \param m23 Fourth element of the third row of the matrix.
       *  \param m30 First element of the fourth row of the matrix.
       *  \param m31 Second element of the fourth row of the matrix.
       *  \param m32 Third element of the fourth row of the matrix.
       *  \param m33 Fourth element of the fourth row of the matrix.
       *  \remarks This constructor can only be used with 4 by 4 matrices, like Mat44f.
       *  \par Example:
       *  \code
       *    Mat44f m44f(  1.0f,  2.0f,  3.0f,  4.0f
       *               ,  5.0f,  6.0f,  7.0f,  8.0f
       *               ,  9.0f, 10.0f, 11.0f, 12.0f
       *               , 13.0f, 14.0f, 15.0f, 16.0f );
       *  \endcode */
      Matnnt( T m00, T m01, T m02, T m03
            , T m10, T m11, T m12, T m13
            , T m20, T m21, T m22, T m23
            , T m30, T m31, T m32, T m33 );

      /*! \brief Constructor for a 4 by 4 matrix out of four vectors of four values each.
       *  \param v0 First row of the matrix.
       *  \param v1 Second row of the matrix.
       *  \param v2 Third row of the matrix.
       *  \param v3 Fourth row of the matrix.
       *  \remarks This constructor can only be used with 4 by 4 matrices, like Mat44f.
       *  \par Example:
       *  \code
       *    Mat44f m44f( Vec3f( xAxis, 0.0f )
       *               , Vec4f( yAxis, 0.0f )
       *               , Vec4f( zAxis, 0.0f )
       *               , Vec4f( trans, 1.0f ) );
       *  \endcode */
      Matnnt( const Vecnt<n,T> & v0, const Vecnt<n,T> & v1, const Vecnt<n,T> & v2, const Vecnt<n,T> & v3 );

      /*! \brief Constructor for a 3 by 3 rotation matrix out of an axis and an angle.
       *  \param axis A reference to the constant axis to rotate about.
       *  \param angle The angle, in radians, to rotate.
       *  \remarks The resulting 3 by 3 matrix is a pure rotation.
       *  \note The behavior is undefined, if \a axis is not normalized.
       *  \par Example:
       *  \code
       *    Mat33f rotZAxisBy45Degrees( Vec3f( 0.0f, 0.0f, 1.0f ), PI/4 );
       *  \endcode */
      Matnnt( const Vecnt<n,T> & axis, T angle );

      /*! \brief Constructor for a 3 by 3 rotation matrix out of a normalized quaternion.
       *  \param ori A reference to the normalized quaternion representing the rotation.
       *  \remarks The resulting 3 by 3 matrix is a pure rotation.
       *  \note The behavior is undefined, if \a ori is not normalized. */
      explicit Matnnt( const Quatt<T> & ori );

      /*! \brief Constructor for a 4 by 4 transformation matrix out of a quaternion and a translation.
       *  \param ori A reference to the normalized quaternion representing the rotational part.
       *  \param trans A reference to the vector representing the translational part.
       *  \note The behavior is undefined, if \ ori is not normalized. */
      Matnnt( const Quatt<T> & ori, const Vecnt<n-1,T> & trans );

      /*! \brief Constructor for a 4 by 4 transformation matrix out of a quaternion, a translation,
       *  and a scaling.
       *  \param ori A reference to the normalized quaternion representing the rotational part.
       *  \param trans A reference to the vector representing the translational part.
       *  \param scale A reference to the vector representing the scaling along the three axis directions.
       *  \note The behavior is undefined, if \ ori is not normalized. */
      Matnnt( const Quatt<T> & ori, const Vecnt<n-1,T> & trans, const Vecnt<n-1,T> & scale );

    public:
      /*! \brief Get a constant pointer to the n times n values of the matrix.
       *  \return A constant pointer to the matrix elements.
       *  \remarks The matrix elements are stored in row-major order. This function returns the
       *  address of the first element of the first row. It is assured, that the other elements of
       *  the matrix follow linearly.
       *  \par Example:
       *  If \c m is a 3 by 3 matrix, m.getPtr() gives a pointer to the 9 elements m00, m01, m02, m10,
       *  m11, m12, m20, m21, m22, in that order. */
      const T * getPtr() const;

      /*! \brief Invert the matrix.
       *  \return \c true, if the matrix was successfully inverted, otherwise \c false.
       *  \note If the matrix was not successfully inverted, the resulting matrix is undefined. */
      bool invert();

      /*! \brief Non-constant subscript operator.
       *  \param i Index of the row to address.
       *  \return A reference to the \a i th row of the matrix. */
      template<typename S>
        Vecnt<n,T> & operator[]( S i );

      /*! \brief Constant subscript operator.
       *  \param i Index of the row to address.
       *  \return A constant reference to the \a i th row of the matrix. */
      template<typename S>
        const Vecnt<n,T> & operator[]( S i ) const;

      /*! \brief Matrix addition and assignment operator.
       *  \param m A constant reference to the matrix to add.
       *  \return A reference to \c this.
       *  \remarks The matrix \a m has to be of the same size as \c this, but may hold values of a
       *  different type. The matrix elements of type \a S of \a m are converted to type \a T and
       *  added to the corresponding matrix elements of \c this. */
      template<typename S>
        Matnnt<n,T> & operator+=( const Matnnt<n,S> & m );

      /*! \brief Matrix subtraction and assignment operator.
       *  \param m A constant reference to the matrix to subtract.
       *  \return A reference to \c this.
       *  \remarks The matrix \a m has to be of the same size as \c this, but may hold values of a
       *  different type. The matrix elements of type \a S of \a m are converted to type \a T and
       *  subtracted from the corresponding matrix elements of \c this. */
      template<typename S>
        Matnnt<n,T> & operator-=( const Matnnt<n,S> & m );

      /*! \brief Scalar multiplication and assignment operator.
       *  \param s A scalar value to multiply with.
       *  \return A reference to \c this.
       *  \remarks The type of \a s may be of different type as the elements of the \c this. \a s is
       *  converted to type \a T and each element of \c this is multiplied with it. */
      template<typename S>
        Matnnt<n,T> & operator*=( S s );

      /*! \brief Matrix multiplication and assignment operator.
       *  \param m A constant reference to the matrix to multiply with.
       *  \return A reference to \c this.
       *  \remarks The matrix multiplication \code *this * m \endcode is calculated and assigned to
       *  \c this. */
      Matnnt<n,T> & operator*=( const Matnnt<n,T> & m );

      /*! \brief Scalar division and assignment operator.
       *  \param s A scalar value to divide by.
       *  \return A reference to \c this.
       *  \remarks The type of \a s may be of different type as the elements of the \c this. \a s is
       *  converted to type \a T and each element of \c this is divided by it.
       *  \note The behavior is undefined if \a s is very close to zero. */
      template<typename S>
        Matnnt<n,T> & operator/=( S s );

    private:
      Vecnt<n,T>  m_mat[n];
  };


  // - - - - -  - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
  // non-member functions
  // - - - - -  - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

  /*! \brief Test if a matrix is the identity.
   *  \param m A constant reference to the matrix to test for identity.
   *  \param eps An optional value giving the allowed epsilon. The default is a type dependent value.
   *  \return \c true, if the matrix is the identity, otherwise \c false.
   *  \remarks A matrix is considered to be the identity, if each of the diagonal elements differ
   *  less than \a eps from one, and each of the other matrix elements differ less than \a eps from
   *  zero.
   *  \sa isNormalized, isNull, isOrthogonal */
  template<unsigned int n, typename T>
    bool isIdentity( const Matnnt<n,T> & m, T eps = std::numeric_limits<T>::epsilon() )
  {
    bool identity = true;
    for ( unsigned int i=0 ; identity && i<n ; ++i )
    {
      for ( unsigned int j=0 ; identity && j<i ; ++j )
      {
        identity = ( abs( m[i][j] ) <= eps );
      }
      if ( identity )
      {
        identity = ( abs( m[i][i] - 1 ) <= eps );
      }
      for ( unsigned int j=i+1 ; identity && j<n ; ++j )
      {
        identity = ( abs( m[i][j] ) <= eps );
      }
    }
    return( identity );
  }

  /*! \brief Test if a matrix is normalized.
   *  \param m A constant reference to the matrix to test.
   *  \param eps An optional value giving the allowed epsilon. The default is a type dependent value.
   *  \return \c true if the matrix is normalized, otherwise \c false.
   *  \remarks A matrix is considered to be normalized, if each row and each column is normalized.
   *  \sa isIdentity, isNull, isOrthogonal */
  template<unsigned int n, typename T>
    bool isNormalized( const Matnnt<n,T> & m, T eps = std::numeric_limits<T>::epsilon() )
  {
    bool normalized = true;
    for ( unsigned int i=0 ; normalized && i<n ; ++i )
    {
      normalized = isNormalized( m[i], eps );
    }
    for ( unsigned int i=0 ; normalized && i<n ; ++i )
    {
      Vecnt<n,T> v;
      for ( unsigned int j=0 ; j<n ; j++ )
      {
        v[j] = m[j][i];
      }
      normalized = isNormalized( v, eps );
    }
    return( normalized );
  }

  /*! \brief Test if a matrix is null.
   *  \param m A constant reference to the matrix to test.
   *  \param eps An optional value giving the allowed epsilon. The default is a type dependent value.
   *  \return \c true if the matrix is null, otherwise \c false.
   *  \remarks A matrix is considered to be null, if each row is null.
   *  \sa isIdentity, isNormalized, isOrthogonal */
  template<unsigned int n, typename T>
    bool isNull( const Matnnt<n,T> & m, T eps = std::numeric_limits<T>::epsilon() )
  {
    bool null = true;
    for ( unsigned int i=0 ; null && i<n ; ++i )
    {
      null = isNull( m[i], eps );
    }
    return( null );
  }

  /*! \brief Test if a matrix is orthogonal.
   *  \param m A constant reference to the matrix to test.
   *  \param eps An optional value giving the allowed epsilon. The default is a type dependent value.
   *  \return \c true, if the matrix is orthogonal, otherwise \c false.
   *  \remarks A matrix is considered to be orthogonal, if each pair of rows and each pair of
   *  columns are orthogonal to each other.
   *  \sa isIdentity, isNormalized, isNull */
  template<unsigned int n, typename T>
    bool isOrthogonal( const Matnnt<n,T> & m, T eps = std::numeric_limits<T>::epsilon() )
  {
    bool orthogonal = true;
    for ( unsigned int i=0 ; orthogonal && i+1<n ; ++i )
    {
      for ( unsigned int j=i+1 ; orthogonal && j<n ; ++j )
      {
        orthogonal = areOrthogonal( m[i], m[j], eps );
      }
    }
    if ( orthogonal )
    {
      Matnnt<n,T> tm = ~m;
      for ( unsigned int i=0 ; orthogonal && i+1<n ; ++i )
      {
        for ( unsigned int j=i+1 ; orthogonal && j<n ; ++j )
        {
          orthogonal = areOrthogonal( tm[i], tm[j], eps );
        }
      }
    }
    return( orthogonal );
  }

  /*! \brief Get the value of the maximal absolute element of a matrix.
   *  \param m A constant reference to a matrix to get the maximal element from.
   *  \return The value of the maximal absolute element of \a m.
   *  \sa minElement */
  template<unsigned int n, typename T>
    T maxElement( const Matnnt<n,T> & m );

  /*! \brief Get the value of the minimal absolute element of a matrix.
   *  \param m A constant reference to a matrix to get the minimal element from.
   *  \return The value of the minimal absolute element of \a m.
   *  \sa maxElement */
  template<unsigned int n, typename T>
    T minElement( const Matnnt<n,T> & m );

  /*! \brief Matrix equality operator.
   *  \param m0 A constant reference to the first matrix to compare.
   *  \param m1 A constant reference to the second matrix to compare.
   *  \return \c true, if \a m0 and \a m1 are equal, otherwise \c false.
   *  \remarks Two matrices are considered to be equal, if each element of \a m0 differs less than
   *  the type dependent epsilon from the the corresponding element of \a m1. */
  template<unsigned int n, typename T>
    bool operator==( const Matnnt<n,T> & m0, const Matnnt<n,T> & m1 );

  /*! \brief Matrix inequality operator.
   *  \param m0 A constant reference to the first matrix to compare.
   *  \param m1 A constant reference to the second matrix to compare.
   *  \return \c true, if \a m0 and \a m1 are not equal, otherwise \c false.
   *  \remarks Two matrices are considered to be not equal, if at least one element of \a m0 differs
   *  more than the type dependent epsilon from the the corresponding element of \a m1. */
  template<unsigned int n, typename T>
    bool operator!=( const Matnnt<n,T> & m0, const Matnnt<n,T> & m1 );

  /*! \brief Matrix transpose operator.
   *  \param m A constant reference to the matrix to transpose.
   *  \return The transposed version of \a m. */
  template<unsigned int n, typename T>
    Matnnt<n,T> operator~( const Matnnt<n,T> & m );

  /*! \brief Matrix addition operator.
   *  \param m0 A constant reference to the first matrix to add.
   *  \param m1 A constant reference to the second matrix to add.
   *  \return A matrix representing the sum of \code m0 + m1 \endcode. */
  template<unsigned int n, typename T>
    Matnnt<n,T> operator+( const Matnnt<n,T> & m0, const Matnnt<n,T> & m1 );

  /*! \brief Matrix negation operator.
   *  \param m A constant reference to the matrix to negate.
   *  \return A matrix representing the negation of \a m. */
  template<unsigned int n, typename T>
    Matnnt<n,T> operator-( const Matnnt<n,T> & m );

  /*! \brief Matrix subtraction operator.
   *  \param m0 A constant reference to the first argument of the subtraction.
   *  \param m1 A constant reference to the second argument of the subtraction.
   *  \return A matrix representing the difference \code m0 - m1 \endcode. */
  template<unsigned int n, typename T>
    Matnnt<n,T> operator-( const Matnnt<n,T> & m0, const Matnnt<n,T> & m1 );

  /*! \brief Scalar multiplication operator.
   *  \param m A constant reference to the matrix to multiply.
   *  \param s The scalar value to multiply with.
   *  \return A matrix representing the product \code m * s \endcode. */
  template<unsigned int n, typename T>
    Matnnt<n,T> operator*( const Matnnt<n,T> & m, T s );

  /*! \brief Scalar multiplication operator.
   *  \param s The scalar value to multiply with.
   *  \param m A constant reference to the matrix to multiply.
   *  \return A matrix representing the product \code s * m \endcode. */
  template<unsigned int n, typename T>
    Matnnt<n,T> operator*( T s, const Matnnt<n,T> & m );

  /*! \brief Vector multiplication operator.
   *  \param m A constant reference to the matrix to multiply.
   *  \param v A constant reference to the vector to multiply with.
   *  \return A vector representing the product \code m * v \endcode. */
  template<unsigned int n, typename T>
    Vecnt<n,T> operator*( const Matnnt<n,T> & m, const Vecnt<n,T> & v );

  /*! \brief Vector multiplication operator.
   *  \param v A constant reference to the vector to multiply with.
   *  \param m A constant reference to the matrix to multiply.
   *  \return A vector representing the product \code v * m \endcode. */
  template<unsigned int n, typename T>
    Vecnt<n,T> operator*( const Vecnt<n,T> & v, const Matnnt<n,T> & m );

  /*! \brief Matrix multiplication operator.
   *  \param m0 A constant reference to the first operand of the multiplication.
   *  \param m1 A constant reference to the second operand of the multiplication.
   *  \return A matrix representing the product \const m0 * m1 \endocde. */
  template<unsigned int n, typename T>
    Matnnt<n,T> operator*( const Matnnt<n,T> & m0, const Matnnt<n,T> & m1 );

  /*! \brief Scalar division operator.
   *  \param m A constant reference to the matrix to divide.
   *  \param s The scalar value to divide by.
   *  \return A matrix representing the matrix \a m divided by \a s. */
  template<unsigned int n, typename T>
    Matnnt<n,T> operator/( const Matnnt<n,T> & m, T s );

  /*! \brief Set a matrix to be the identity.
   *  \param m The matrix to set to identity.
   *  \remarks Each diagonal element of \a m is set to one, each other element is set to zero. */
  template<unsigned int n, typename T>
    void setIdentity( Matnnt<n,T> & m );


  // - - - - -  - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
  // non-member functions, specialized for n == 3
  // - - - - -  - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

  /*! \brief Determine the determinant of a 3 by 3 matrix.
   *  \param m A constant reference to the matrix to determine the determinant from.
   *  \return The determinant of \a m. */
  template<typename T>
    T determinant( const Matnnt<3,T> & m );

  /*! \brief Test if a 3 by 3 matrix represents a rotation.
   *  \param m A constant reference to the matrix to test.
   *  \param eps An optional value giving the allowed epsilon. The default is a type dependent value.
   *  \return \c true, if the matrix represents a rotation, otherwise \c false.
   *  \remarks A 3 by 3 matrix is considered to be a rotation, if it is normalized, orthogonal, and its
   *  determinant is one.
   *  \sa isIdentity, isNull, isNormalized, isOrthogonal */
  template<typename T>
    bool isRotation( const Matnnt<3,T> & m, T eps = 9 * std::numeric_limits<T>::epsilon() )
  {
    return(   isNormalized( m, eps )
          &&  isOrthogonal( m, eps )
          && ( abs( determinant( m ) - 1 ) <= eps ) );
  }

  /*! \brief Set the values of a 3 by 3 matrix by nine scalars
   *  \param m A reference to the matrix to set with the nine values.
   *  \param m00 The scalar for the first value in the first row.
   *  \param m01 The scalar for the second value in the first row.
   *  \param m02 The scalar for the third value in the first row.
   *  \param m10 The scalar for the first value in the second row.
   *  \param m11 The scalar for the second value in the second row.
   *  \param m12 The scalar for the third value in the second row.
   *  \param m20 The scalar for the first value in the third row.
   *  \param m21 The scalar for the second value in the third row.
   *  \param m22 The scalar for the third value in the third row.
   *  \return A reference to \a m. */
  template<typename T>
    Matnnt<3,T> & setMat( Matnnt<3,T> & m, T m00, T m01, T m02
                                         , T m10, T m11, T m12
                                         , T m20, T m21, T m22 );

  /*! \brief Set the values of a 3 by 3 matrix by three vectors.
   *  \param m A reference to the matrix to set with the three vectors.
   *  \param v0 A constant reference to the vector to set as the first row of \a m.
   *  \param v1 A constant reference to the vector to set as the second row of \a m.
   *  \param v2 A constant reference to the vector to set as the third row of \a m.
   *  \return A reference to \a m. */
  template<typename T>
    Matnnt<3,T> & setMat( Matnnt<3,T> & m, const Vecnt<3,T> & v0
                                         , const Vecnt<3,T> & v1
                                         , const Vecnt<3,T> & v2 );

  /*! \brief Set the values of a 3 by 3 matrix using a normalized quaternion.
   *  \param m A reference to the matrix to set.
   *  \param q A constant reference to the normalized quaternion to use.
   *  \return A reference to \a m.
   *  \remarks The matrix is set to represent the same rotation as the normalized quaternion \a q.
   *  \note The behavior is undefined if \a q is not normalized. */
  template<typename T>
    Matnnt<3,T> & setMat( Matnnt<3,T> & m, const Quatt<T> & q );

  /*! \brief Set the values of a 3 by 3 matrix using a normalized rotation axis and an angle.
   *  \param m A reference to the matrix to set.
   *  \param axis A constant reference to the normalized rotation axis.
   *  \param angle The angle in radians to rotate around \a axis.
   *  \return A reference to \a m.
   *  \remarks The matrix is set to represent the rotation by \a angle around \a axis.
   *  \note The behavior is undefined if \a axis is not normalized. */
  template<typename T>
    Matnnt<3,T> & setMat( Matnnt<3,T> & m, const Vecnt<3,T> & axis, T angle );


  // - - - - -  - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
  // non-member functions, specialized for n == 3, T == float
  // - - - - -  - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

  /*! \brief Decompose a 3 by 3 matrix.
   *  \param m A constant reference to the matrix to decompose.
   *  \param orientation A reference to the quaternion getting the rotational part of the matrix.
   *  \param scaling A reference to the vector getting the scaling part of the matrix.
   *  \param scaleOrientation A reference to the quaternion getting the orientation of the scaling.
   *  \note The behavior is undefined, if the determinant of \a m is too small, or the rank of \a m
   *  is less than three. */
  void decompose( const Matnnt<3,float> &m, Quatt<float> &orientation
                         , Vecnt<3,float> &scaling, Quatt<float> &scaleOrientation );


  // - - - - -  - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
  // non-member functions, specialized for n == 4
  // - - - - -  - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

  /*! \brief Set the values of a 4 by 4 matrix by sixteen scalars
   *  \param m A reference to the matrix to set with the sixteen values.
   *  \param m00 The scalar for the first value in the first row.
   *  \param m01 The scalar for the second value in the first row.
   *  \param m02 The scalar for the third value in the first row.
   *  \param m03 The scalar for the fourth value in the first row.
   *  \param m10 The scalar for the first value in the second row.
   *  \param m11 The scalar for the second value in the second row.
   *  \param m12 The scalar for the third value in the second row.
   *  \param m13 The scalar for the fourth value in the second row.
   *  \param m20 The scalar for the first value in the third row.
   *  \param m21 The scalar for the second value in the third row.
   *  \param m22 The scalar for the third value in the third row.
   *  \param m23 The scalar for the fourth value in the third row.
   *  \param m30 The scalar for the first value in the fourth row.
   *  \param m31 The scalar for the second value in the fourth row.
   *  \param m32 The scalar for the third value in the fourth row.
   *  \param m33 The scalar for the fourth value in the fourth row.
   *  \return A reference to \a m. */
  template<typename T>
    Matnnt<4,T> & setMat( Matnnt<4,T> & m, T m00, T m01, T m02, T m03
                                         , T m10, T m11, T m12, T m13
                                         , T m20, T m21, T m22, T m23
                                         , T m30, T m31, T m32, T m33 );

  /*! \brief Set the values of a 4 by 4 matrix by four vectors.
   *  \param m A reference to the matrix to set with the four vectors.
   *  \param v0 A constant reference to the vector to set as the first row of \a m.
   *  \param v1 A constant reference to the vector to set as the second row of \a m.
   *  \param v2 A constant reference to the vector to set as the third row of \a m.
   *  \param v3 A constant reference to the vector to set as the fourth row of \a m.
   *  \return A reference to \a m. */
  template<typename T>
    Matnnt<4,T> & setMat( Matnnt<4,T> & m, const Vecnt<4,T> & v0
                                         , const Vecnt<4,T> & v1
                                         , const Vecnt<4,T> & v2
                                         , const Vecnt<4,T> & v3 );

  /*! \brief Set the values of a 4 by 4 matrix by the constituents of a transformation.
   *  \param m A reference to the matrix to set.
   *  \param ori A constant reference of the rotation part of the transformation.
   *  \param trans An optional constant reference to the translational part of the transformation. The
   *  default is a null vector.
   *  \param scale An optional constant reference to the scaling part of the transformation. The default
   *  is the identity scaling.
   *  \return A reference to \a m. */
  template<typename T>
    Matnnt<4,T> & setMat( Matnnt<4,T> & m, const Quatt<T> & ori
                                         , const Vecnt<3,T> & trans = Vecnt<3,T>(0,0,0)
                                         , const Vecnt<3,T> & scale = Vecnt<3,T>(1,1,1) )
  {
    Matnnt<3,T>  m3( ori );
    m[0] = Vecnt<4,T>( scale[0] * m3[0], 0 );
    m[1] = Vecnt<4,T>( scale[1] * m3[1], 0 );
    m[2] = Vecnt<4,T>( scale[2] * m3[2], 0 );
    m[3] = Vecnt<4,T>( trans, 1 );
    return( m );
  }

  /*! \brief Set the values of a 4 by 4 matrix by the constituents of a transformation.
   *  \param m A reference to the matrix to set.
   *  \param trans An constant reference to the translational part of the transformation. 
   *  \return A reference to \a m. */
  template<typename T>
    Matnnt<4,T> & setTransMat( Matnnt<4,T> & m, const Vecnt<3,T> & trans )
  {
    m[0] = Vecnt<4,T>( 1, 0, 0, 0 );
    m[1] = Vecnt<4,T>( 0, 1, 0, 0 );
    m[2] = Vecnt<4,T>( 0, 0, 1, 0 );
    m[3] = Vecnt<4,T>( trans, 1 );
    return( m );
  }

  // - - - - -  - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
  // non-member functions, specialized for n == 4, T == float
  // - - - - -  - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

  /*! \brief Decompose a 4 by 4 matrix of floats.
   *  \param m A constant reference to the matrix to decompose.
   *  \param translation A reference to the vector getting the translational part of the matrix.
   *  \param orientation A reference to the quaternion getting the rotational part of the matrix.
   *  \param scaling A reference to the vector getting the scaling part of the matrix.
   *  \param scaleOrientation A reference to the quaternion getting the orientation of the scaling.
   *  \note The behavior is undefined, if the determinant of \a m is too small.
   *  \note Currently, the behavior is undefined, if the rank of \a m is less than three. */
  void decompose( const Matnnt<4,float> &m, Vecnt<3,float> &translation
                         , Quatt<float> &orientation, Vecnt<3,float> &scaling
                         , Quatt<float> &scaleOrientation );


  // - - - - -  - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
  // Convenience type definitions
  // - - - - -  - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
  typedef Matnnt<3,float>   Mat33f;
  typedef Matnnt<3,double>  Mat33d;
  typedef Matnnt<4,float>   Mat44f;
  typedef Matnnt<4,double>  Mat44d;


  // - - - - -  - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
  // inlined member functions
  // - - - - -  - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

  template<unsigned int n, typename T>
  inline Matnnt<n,T>::Matnnt()
  {
  }

  template<unsigned int n, typename T>
  inline Matnnt<n,T>::Matnnt( T m00, T m01, T m02
                            , T m10, T m11, T m12
                            , T m20, T m21, T m22 )
  {
    setMat( *this, m00, m01, m02
                 , m10, m11, m12
                 , m20, m21, m22 );
  }

  template<unsigned int n, typename T>
  inline Matnnt<n,T>::Matnnt( const Vecnt<n,T> & v0, const Vecnt<n,T> & v1, const Vecnt<n,T> & v2 )
  {
    setMat( *this, v0, v1, v2 );
  }

  template<unsigned int n, typename T>
  inline Matnnt<n,T>::Matnnt( T m00, T m01, T m02, T m03
                            , T m10, T m11, T m12, T m13
                            , T m20, T m21, T m22, T m23
                            , T m30, T m31, T m32, T m33 )
  {
    setMat( *this, m00, m01, m02, m03
                 , m10, m11, m12, m13
                 , m20, m21, m22, m23
                 , m30, m31, m32, m33 );
  }

  template<unsigned int n, typename T>
  template<unsigned int m, typename S>
  inline Matnnt<n,T>::Matnnt( const Matnnt<m,S> & rhs )
  {
	unsigned int min_c = (n < m ? n : m);
    for ( unsigned int i=0 ; i<min_c ; ++i )
    {
      m_mat[i] = Vecnt<n,T>(rhs[i]);
    }
  }

  template<unsigned int n, typename T>
  inline Matnnt<n,T>::Matnnt( const Vecnt<n,T> & v0
                            , const Vecnt<n,T> & v1
                            , const Vecnt<n,T> & v2
                            , const Vecnt<n,T> & v3 )
  {
    setMat( *this, v0, v1, v2, v3 );
  }

  template<unsigned int n, typename T>
  inline Matnnt<n,T>::Matnnt( const Vecnt<n,T> & axis, T angle )
  {
    setMat( *this, axis, angle );
  }

  template<unsigned int n, typename T>
  inline Matnnt<n,T>::Matnnt( const Quatt<T> & ori )
  {
    setMat( *this, ori );
  }

  template<unsigned int n, typename T>
  inline Matnnt<n,T>::Matnnt( const Quatt<T> & ori, const Vecnt<n-1,T> & trans )
  {
    setMat( *this, ori, trans );
  }

  template<unsigned int n, typename T>
  inline Matnnt<n,T>::Matnnt( const Quatt<T> & ori, const Vecnt<n-1,T> & trans, const Vecnt<n-1,T> & scale )
  {
    setMat( *this, ori, trans, scale );
  }

  template<unsigned int n, typename T>
  inline const T * Matnnt<n,T>::getPtr() const
  {
    return( m_mat[0].getPtr() );
  }

  template<unsigned int n, typename T>
  bool Matnnt<n,T>::invert()
  {
    unsigned int p[n];

    bool ok = true;
    for ( unsigned int k=0 ; ok && k<n ; ++k )
    {
      T nv_max(0);
      p[k] = 0;
      for ( unsigned int i=k ; ok && i<n ; ++i )
      {
        T s(0);
        for ( unsigned int j=k ; j<n ; ++j )
        {
          s += abs( m_mat[i][j] );
        }
        ok = ( std::numeric_limits<T>::epsilon() < abs(s) );
        if ( ok )
        {
          T q = abs( m_mat[i][k] ) / s;
          if ( q > nv_max )
          {
            nv_max = q;
            p[k] = i;
          }
        }
      }

      ok = ( std::numeric_limits<T>::epsilon() < nv_max );
      if ( ok )
      {
        if ( p[k] != k )
        {
          for ( unsigned int j=0 ; j<n ; ++j )
          {
            std::swap( m_mat[k][j], m_mat[p[k]][j] );
          }
        }

        T pivot = m_mat[k][k];
        ok = ( std::numeric_limits<T>::epsilon() < abs( pivot ) );
        if ( ok )
        {
          for ( unsigned int j=0 ; j<n ; ++j )
          {
            if ( j != k )
            {
              m_mat[k][j] /= - pivot;
              for ( unsigned int i=0 ; i<n ; ++i )
              {
                if ( i != k )
                {
                  m_mat[i][j] += m_mat[i][k] * m_mat[k][j];
                }
              }
            }
          }

          for ( unsigned int i=0 ; i<n ; ++i )
          {
            m_mat[i][k] /= pivot;
          }
          m_mat[k][k] = 1.0f / pivot;
        }
      }
    }

    if ( ok )
    {
      for ( unsigned int k=n-2 ; k<n ; --k )  //  NOTE: ( unsigned int k < n ) <=> ( int k >= 0 )
      {
        if ( p[k] != k )
        {
          for ( unsigned int i=0 ; i<n ; ++i )
          {
            std::swap( m_mat[i][k], m_mat[i][p[k]] );
          }
        }
      }
    }

    return( ok );
  }

  template<unsigned int n, typename T>
  template<typename S>
  inline Vecnt<n,T> & Matnnt<n,T>::operator[]( S i )
  {
    //NVSG_CTASSERT( std::numeric_limits<S>::is_integer );
    //NVSG_ASSERT( i < n );
    return( m_mat[i] );
  }

  template<unsigned int n, typename T>
  template<typename S>
  inline const Vecnt<n,T> & Matnnt<n,T>::operator[]( S i ) const
  {
    //NVSG_CTASSERT( std::numeric_limits<S>::is_integer );
    //NVSG_ASSERT( i < n );
    return( m_mat[i] );
  }

  template<unsigned int n, typename T>
  template<typename S>
  inline Matnnt<n,T> & Matnnt<n,T>::operator+=( const Matnnt<n,S> & rhs )
  {
    for ( unsigned int i=0 ; i<n ; ++i )
    {
      m_mat[i] += rhs[i];
    }
    return( *this );
  }

  template<unsigned int n, typename T>
  template<typename S>
  inline Matnnt<n,T> & Matnnt<n,T>::operator-=( const Matnnt<n,S> & rhs )
  {
    for ( unsigned int i=0 ; i<n ; ++i )
    {
      m_mat[i] -= rhs[i];
    }
    return( *this );
  }

  template<unsigned int n, typename T>
  template<typename S>
  inline Matnnt<n,T> & Matnnt<n,T>::operator*=( S s )
  {
    for ( unsigned int i=0 ; i<n ; ++i )
    {
      m_mat[i] *= s;
    }
    return( *this );
  }

  template<unsigned int n, typename T>
  inline Matnnt<n,T> & Matnnt<n,T>::operator*=( const Matnnt<n,T> & rhs )
  {
    *this = *this * rhs;
    return( *this );
  }

  template<unsigned int n, typename T>
  template<typename S>
  inline Matnnt<n,T> & Matnnt<n,T>::operator/=( S s )
  {
    for ( unsigned int i=0 ; i<n ; ++i )
    {
      m_mat[i] /= s;
    }
    return( *this );
  }


  // - - - - -  - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
  // inlined non-member functions
  // - - - - -  - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
  template<unsigned int n, typename T>
  inline T maxElement( const Matnnt<n,T> & m )
  {
    T me = maxElement( m[0] );
    for ( unsigned int i=1 ; i<n ; ++i )
    {
      T t = maxElement( m[i] );
      if ( me < t )
      {
        me = t;
      }
    }
    return( me );
  }

  template<unsigned int n, typename T>
  inline T minElement( const Matnnt<n,T> & m )
  {
    T me = minElement( m[0] );
    for ( unsigned int i=1 ; i<n ; ++i )
    {
      T t = minElement( m[i] );
      if ( t < me )
      {
        me = t;
      }
    }
    return( me );
  }

  template<unsigned int n, typename T>
  inline bool operator==( const Matnnt<n,T> & m0, const Matnnt<n,T> & m1 )
  {
    bool eq = true;
    for ( unsigned int i=0 ; i<n && eq ; ++i )
    {
      eq = ( m0[i] == m1[i] );
    }
    return( eq );
  }

  template<unsigned int n, typename T>
  inline bool operator!=( const Matnnt<n,T> & m0, const Matnnt<n,T> & m1 )
  {
    return( ! ( m0 == m1 ) );
  }

  template<unsigned int n, typename T>
  inline Matnnt<n,T> operator~( const Matnnt<n,T> & m )
  {
    Matnnt<n,T> ret;
    for ( unsigned int i=0 ; i<n ; ++i )
    {
      for ( unsigned int j=0 ; j<n ; ++j )
      {
        ret[i][j] = m[j][i];
      }
    }
    return( ret );
  }

  template<unsigned int n, typename T>
  inline Matnnt<n,T> operator+( const Matnnt<n,T> & m0, const Matnnt<n,T> & m1 )
  {
    Matnnt<n,T> ret(m0);
    ret += m1;
    return( ret );
  }

  template<unsigned int n, typename T>
  inline Matnnt<n,T> operator-( const Matnnt<n,T> & m )
  {
    Matnnt<n,T> ret;
    for ( unsigned int i=0 ; i<n ; ++i )
    {
      ret[i] = -m[i];
    }
    return( ret );
  }

  template<unsigned int n, typename T>
  inline Matnnt<n,T> operator-( const Matnnt<n,T> & m0, const Matnnt<n,T> & m1 )
  {
    Matnnt<n,T> ret(m0);
    ret -= m1;
    return( ret );
  }

  template<unsigned int n, typename T>
  inline Matnnt<n,T> operator*( const Matnnt<n,T> & m, T s )
  {
    Matnnt<n,T> ret(m);
    ret *= s;
    return( ret );
  }

  template<unsigned int n, typename T>
  inline Matnnt<n,T> operator*( T s, const Matnnt<n,T> & m )
  {
    return( m * s );
  }

  template<unsigned int n, typename T>
  inline Vecnt<n,T> operator*( const Matnnt<n,T> & m, const Vecnt<n,T> & v )
  {
    Vecnt<n,T> ret;
    for ( unsigned int i=0 ; i<n ; ++i )
    {
      ret[i] = m[i] * v;
    }
    return( ret );
  }

  template<unsigned int n, typename T>
  inline Vecnt<n,T> operator*( const Vecnt<n,T> & v, const Matnnt<n,T> & m )
  {
    Vecnt<n,T> ret;
    for ( unsigned int i=0 ; i<n ; ++i )
    {
      ret[i] = 0;
      for ( unsigned int j=0 ; j<n ; ++j )
      {
        ret[i] += v[j] * m[j][i];
      }
    }
    return( ret );
  }

  template<unsigned int n, typename T>
  inline Matnnt<n,T> operator*( const Matnnt<n,T> & m0, const Matnnt<n,T> & m1 )
  {
    Matnnt<n,T> ret;
    for ( unsigned int i=0 ; i<n ; ++i )
    {
      for ( unsigned int j=0 ; j<n ; ++j )
      {
        ret[i][j] = 0;
        for ( unsigned int k=0 ; k<n ; ++k )
        {
          ret[i][j] += m0[i][k] * m1[k][j];
        }
      }
    }
    return( ret );
  }

  template<>
  inline Matnnt<4, float> operator*(const Matnnt<4,float> & m0, const Matnnt<4,float> & m1)
  {
    return Matnnt<4,float> (
      m0[0][0]*m1[0][0] + m0[0][1]*m1[1][0] + m0[0][2]*m1[2][0] + m0[0][3]*m1[3][0],
      m0[0][0]*m1[0][1] + m0[0][1]*m1[1][1] + m0[0][2]*m1[2][1] + m0[0][3]*m1[3][1],
      m0[0][0]*m1[0][2] + m0[0][1]*m1[1][2] + m0[0][2]*m1[2][2] + m0[0][3]*m1[3][2],
      m0[0][0]*m1[0][3] + m0[0][1]*m1[1][3] + m0[0][2]*m1[2][3] + m0[0][3]*m1[3][3],

      m0[1][0]*m1[0][0] + m0[1][1]*m1[1][0] + m0[1][2]*m1[2][0] + m0[1][3]*m1[3][0],
      m0[1][0]*m1[0][1] + m0[1][1]*m1[1][1] + m0[1][2]*m1[2][1] + m0[1][3]*m1[3][1],
      m0[1][0]*m1[0][2] + m0[1][1]*m1[1][2] + m0[1][2]*m1[2][2] + m0[1][3]*m1[3][2],
      m0[1][0]*m1[0][3] + m0[1][1]*m1[1][3] + m0[1][2]*m1[2][3] + m0[1][3]*m1[3][3],

      m0[2][0]*m1[0][0] + m0[2][1]*m1[1][0] + m0[2][2]*m1[2][0] + m0[2][3]*m1[3][0],
      m0[2][0]*m1[0][1] + m0[2][1]*m1[1][1] + m0[2][2]*m1[2][1] + m0[2][3]*m1[3][1],
      m0[2][0]*m1[0][2] + m0[2][1]*m1[1][2] + m0[2][2]*m1[2][2] + m0[2][3]*m1[3][2],
      m0[2][0]*m1[0][3] + m0[2][1]*m1[1][3] + m0[2][2]*m1[2][3] + m0[2][3]*m1[3][3],

      m0[3][0]*m1[0][0] + m0[3][1]*m1[1][0] + m0[3][2]*m1[2][0] + m0[3][3]*m1[3][0],
      m0[3][0]*m1[0][1] + m0[3][1]*m1[1][1] + m0[3][2]*m1[2][1] + m0[3][3]*m1[3][1],
      m0[3][0]*m1[0][2] + m0[3][1]*m1[1][2] + m0[3][2]*m1[2][2] + m0[3][3]*m1[3][2],
      m0[3][0]*m1[0][3] + m0[3][1]*m1[1][3] + m0[3][2]*m1[2][3] + m0[3][3]*m1[3][3]
    );
  }

  template<unsigned int n, typename T>
  inline Matnnt<n,T> operator/( const Matnnt<n,T> & m, T s )
  {
    Matnnt<n,T> ret(m);
    ret /= s;
    return( ret );
  }

  template<unsigned int n, typename T>
  void setIdentity( Matnnt<n,T> & m )
  {
    for ( unsigned int i=0 ; i<n ; ++i )
    {
      for ( unsigned int j=0 ; j<i ; ++j )
      {
        m[i][j] = T(0);
      }
      m[i][i] = T(1);
      for ( unsigned int j=i+1 ; j<n ; ++j )
      {
        m[i][j] = T(0);
      }
    }
  }


  // - - - - -  - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
  // inlined non-member functions, specialized for n == 3
  // - - - - -  - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
  template<typename T>
  inline T determinant( const Matnnt<3,T> & m )
  {
    return( m[0] * ( m[1] ^ m[2] ) );
  }

  template<typename T>
  inline Matnnt<3,T> & setMat( Matnnt<3,T> & m, T m00, T m01, T m02
                                              , T m10, T m11, T m12
                                              , T m20, T m21, T m22 )
  {
    m[0][0] = m00;  m[0][1] = m01;  m[0][2] = m02;
    m[1][0] = m10;  m[1][1] = m11;  m[1][2] = m12;
    m[2][0] = m20;  m[2][1] = m21;  m[2][2] = m22;
    return( m );
  }

  template<typename T>
  inline Matnnt<3,T> & setMat( Matnnt<3,T> & m, const Vecnt<3,T> & v0
                                              , const Vecnt<3,T> & v1
                                              , const Vecnt<3,T> & v2 )
  {
    m[0] = v0;  m[1] = v1;  m[2] = v2;
    return( m );
  }

  template<typename T>
  Matnnt<3,T> & setMat( Matnnt<3,T> & m, const Vecnt<3,T> & axis, T angle )
  {
    //NVSG_PRIVATE_ASSERT( isNormalized( axis ) );
    T c = cos( angle );
    T s = sin( angle );
    //NVSG_PRIVATE_ASSERT( abs( s * s + c * c - 1 ) <= std::numeric_limits<T>::epsilon() );
    T t = 1 - c;
    T x = axis[0];
    T y = axis[1];
    T z = axis[2];

    m[0] = Vecnt<3,T>( t * x * x + c,     t * x * y + s * z, t * x * z - s * y );
    m[1] = Vecnt<3,T>( t * x * y - s * z, t * y * y + c,     t * y * z + s * x );
    m[2] = Vecnt<3,T>( t * x * z + s * y, t * y * z - s * x, t * z * z + c     );

    //NVSG_PRIVATE_ASSERT( isRotation( m ) );
    return( m );
  }

  template<typename T>
  inline Matnnt<3,T> & setMat( Matnnt<3,T> & m, const Quatt<T> & q )
  {
    //NVSG_PRIVATE_ASSERT( isNormalized( q ) );

    T x = q[0];
    T y = q[1];
    T z = q[2];
    T w = q[3];

    m[0] = Vecnt<3,T>( 1 - 2 * ( y * y + z * z ), 2 * ( x * y + z * w ),     2 * ( x * z - y * w )     );
    m[1] = Vecnt<3,T>( 2 * ( x * y - z * w ),     1 - 2 * ( x * x + z * z ), 2 * ( y * z + x * w )     );
    m[2] = Vecnt<3,T>( 2 * ( x * z + y * w ),     2 * ( y * z - x * w ),     1 - 2 * ( x * x + y * y ) );

    //NVSG_PRIVATE_ASSERT( isRotation( m ) );
    return( m );
  }


  // - - - - -  - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
  // inlined non-member functions, specialized for n == 4
  // - - - - -  - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
  template<typename T>
  inline Matnnt<4,T> & setMat( Matnnt<4,T> & m, T m00, T m01, T m02, T m03
                                              , T m10, T m11, T m12, T m13
                                              , T m20, T m21, T m22, T m23
                                              , T m30, T m31, T m32, T m33 )
  {
    m[0][0] = m00;  m[0][1] = m01;  m[0][2] = m02;  m[0][3] = m03;
    m[1][0] = m10;  m[1][1] = m11;  m[1][2] = m12;  m[1][3] = m13;
    m[2][0] = m20;  m[2][1] = m21;  m[2][2] = m22;  m[2][3] = m23;
    m[3][0] = m30;  m[3][1] = m31;  m[3][2] = m32;  m[3][3] = m33;
    return( m );
  }

  template<typename T>
  inline Matnnt<4,T> & setMat( Matnnt<4,T> & m, const Vecnt<4,T> & v0
                                              , const Vecnt<4,T> & v1
                                              , const Vecnt<4,T> & v2
                                              , const Vecnt<4,T> & v3 )
  {
    m[0] = v0;  m[1] = v1;  m[2] = v2;  m[3] = v3;
    return( m );
  }

  // - - - - -  - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
  // inlined non-member functions, specialized for n == 4, T == float
  // - - - - -  - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
  inline void decompose( const Matnnt<4,float> &m, Vecnt<3,float> &translation
                       , Quatt<float> &orientation, Vecnt<3,float> &scaling
                       , Quatt<float> &scaleOrientation )
  {
    translation = Vecnt<3,float>( m[3] );
    Matnnt<3,float> m33( m );
    decompose( m33, orientation, scaling, scaleOrientation );
  }



  //! global identity matrix.
  extern const Mat44f  cIdentity44f;

} // namespace nvmath

#endif
