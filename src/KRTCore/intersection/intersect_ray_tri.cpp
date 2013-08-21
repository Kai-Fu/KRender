#include "intersect_ray_tri.h"
#include <xmmintrin.h>
#include <tmmintrin.h>


/* Ray-Triangle Intersection Test Routines          */
/* Different optimizations of my and Ben Trumbore's */
/* code from journals of graphics tools (JGT)       */
/* http://www.acm.org/jgt/                          */
/* by Tomas Moller, May 2000                        */

#define EPSILON 0.000001
#define CROSS(dest,v1,v2) \
          dest[0]=v1[1]*v2[2]-v1[2]*v2[1]; \
          dest[1]=v1[2]*v2[0]-v1[0]*v2[2]; \
          dest[2]=v1[0]*v2[1]-v1[1]*v2[0];
#define DOT(v1,v2) (v1[0]*v2[0]+v1[1]*v2[1]+v1[2]*v2[2])
#define SUB(dest,v1,v2) \
          dest[0]=v1[0]-v2[0]; \
          dest[1]=v1[1]-v2[1]; \
          dest[2]=v1[2]-v2[2]; 

/* the original jgt code */
int intersect_triangle(const float* orig, const float* dir,
		       const float* vert0, const float* vert1, const float* vert2,
		       float *t, float *u, float *v)
{
   float edge1[3], edge2[3], tvec[3], pvec[3], qvec[3], nface[3];
   float det,inv_det;

   /* find vectors for two edges sharing vert0 */
   SUB(edge1, vert1, vert0);
   SUB(edge2, vert2, vert0);

   /* begin calculating determinant - also used to calculate U parameter */
   CROSS(pvec, dir, edge2);
   CROSS(nface, edge1, edge2);
   if (DOT(nface, dir) > 0)
	   return 0; // Backward face, ignore it

   /* if determinant is near zero, ray lies in plane of triangle */
   det = DOT(edge1, pvec);

   if (det > -EPSILON && det < EPSILON)
     return 0;
   inv_det = 1.0f / det;

   /* calculate distance from vert0 to ray origin */
   SUB(tvec, orig, vert0);

   /* calculate U parameter and test bounds */
   *u = DOT(tvec, pvec) * inv_det;
   if (*u < 0.0 || *u > 1.0)
     return 0;

   /* prepare to test V parameter */
   CROSS(qvec, tvec, edge1);

   /* calculate V parameter and test bounds */
   *v = DOT(dir, qvec) * inv_det;
   if (*v < 0.0 || *u + *v > 1.0)
     return 0;

   /* calculate t, ray intersects triangle */
   *t = DOT(edge2, qvec) * inv_det;

   return 1;
}



bool RayIntersect(const float* ray_org, const float* ray_dir, const KTriVertPos2& tri, float cur_t, RayTriIntersect& out_info)
{
	float t = 0;
	float u, v;
	KVec3 v0, v1, v2;

	if (tri.mIsMoving) {
		v0 = nvmath::lerp(cur_t, tri.mVertPos[0], tri.mVertPos_Ending[0]);
		v1 = nvmath::lerp(cur_t, tri.mVertPos[1], tri.mVertPos_Ending[1]);
		v2 = nvmath::lerp(cur_t, tri.mVertPos[2], tri.mVertPos_Ending[2]);
	}
	else {
		v0 = tri.mVertPos[0];
		v1 = tri.mVertPos[1];
		v2 = tri.mVertPos[2];
	}

	if (intersect_triangle(ray_org, ray_dir, 
		(float*)&v0, (float*)&v1, (float*)&v2, 
		&t, &u, &v)) {
			if (t > 0 && out_info.ray_t > t) {
				out_info.ray_t = t;
				out_info.u = u;
				out_info.v = v;
				out_info.w = 1.0f - u - v;
				return true;
			}
			else
				return false;
	}
	else
		return false;
}

// The following code is from web site: http://lucille.atso-net.jp/svn/angelina/isect/wald/raytri.cpp
// http://code.google.com/p/raytracersquare/source/browse/RT2_branch/Intersections.cpp
#include <stdio.h>

/* config */
//#define ENABLE_EARLY_EXIT


static void normalize(float v[3])
{
	float norm, inv_norm;

	norm = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
	norm = sqrt(norm);

	if (norm > 1.0e-5) {
		inv_norm = 1.0f / norm;
		v[0] *= inv_norm;
		v[1] *= inv_norm;
		v[2] *= inv_norm;
	}
}

static void vcross(float v[3], float a[3], float b[3])
{
	v[0] = a[1] * b[2] - a[2] * b[1];
	v[1] = a[2] * b[0] - a[0] * b[2];
	v[2] = a[0] * b[1] - a[1] * b[0];

	normalize(v);
}



