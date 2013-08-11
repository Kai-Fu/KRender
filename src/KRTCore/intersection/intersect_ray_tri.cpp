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



bool RayIntersect(const KRay& ray, const KTriVertPos2& tri, float cur_t, UINT32 tri_id, IntersectContext& ctx)
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

	if (intersect_triangle(ray.GetOrg().getPtr(), ray.GetDir().getPtr(), 
		(float*)&v0, (float*)&v1, (float*)&v2, 
		&t, &u, &v)) {
			if (t > 0 && ctx.t > t) {
				ctx.t = t;
				ctx.u = u;
				ctx.v = v;
				ctx.w = 1.0f - u - v;
				ctx.tri_id = tri_id;
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

// should be 16 byte aligned
static const UINT32 modulo3[]  = {0, 1, 2, 0, 1};

inline static bool intersect_one_ray(const KAccelTriangleOpt &acc, const KRay &ray, IntersectContext& ctx)
{
	// determine U and V axii         
	const int uAxis = modulo3[acc.k + 1];        
	const int vAxis = modulo3[acc.k + 2];          
	
	// start high-latency division as early as possible         
	const float nd = 1.0f / (ray.GetDir()[acc.k] + acc.n_u * ray.GetDir()[uAxis] + acc.n_v * ray.GetDir()[vAxis]); 
	if (nd * acc.E_F > 0)
		return false; // Backward face, skip it.

	const float f = (acc.n_d - ray.GetOrg()[acc.k] - acc.n_u * ray.GetOrg()[uAxis] - acc.n_v * ray.GetOrg()[vAxis]) * nd;                  
	
	// check for valid distance.         
	if (f < EPSILON || f > ctx.t) 
		return false;          
	
	// compute hitpoint positions on uv plane         
	const float hu = (ray.GetOrg()[uAxis] + f * ray.GetDir()[uAxis]);         
	const float hv = (ray.GetOrg()[vAxis] + f * ray.GetDir()[vAxis]);                  
	// check first barycentric coordinate         
	ctx.u = (hv * acc.b_nu + hu * acc.b_nv + acc.b_d);                  
	if (ctx.u <= 0)         return false;         
	
	// check second barycentric coordinate         
	ctx.v = (hv * acc.c_nu + hu * acc.c_nv + acc.c_d);          
	//      std::cout << "Fast: " << mue << std::endl;                  
	
	if (ctx.v <= 0.)         return false;         
	// check third barycentric coordinate         
	if (ctx.u + ctx.v >= 1.)         return false;         
	// have a valid hitpoint here. store it.  
	ctx.t = f;
	return true; 
}

#define X 0
#define Y 1
#define Z 2
void PrecomputeAccelTri(const KTriVertPos1& tri, UINT32 tri_id, KAccelTriangleOpt &triAccel)
{
	// calculate triAccel-struture         
	KVec3 c = tri.mVertPos[1] - tri.mVertPos[0];
	KVec3 b = tri.mVertPos[2] - tri.mVertPos[0];                 
	// calculate normal-vector         
	KVec3 normal = c ^ b;

	//projection-dimension         
	triAccel.k = (fabs(normal[0]) > fabs(normal[1])) ? ((fabs(normal[0]) > fabs(normal[2])) ? X : Z) : ((fabs(normal[1]) > fabs(normal[2])) ? Y : Z);                  
	KVec3 nn = normal / normal[triAccel.k];                  
	// n_u, n_v and n_d         
	const int uAxis = modulo3[triAccel.k + 1];         
	const int vAxis = modulo3[triAccel.k + 2];         
	triAccel.n_u = nn[uAxis];         
	triAccel.n_v = nn[vAxis];    
	KVec3 tempVec(tri.mVertPos[0][X], tri.mVertPos[0][Y], tri.mVertPos[0][Z]);
	triAccel.n_d = tempVec * nn;          
	
	// first line equation         
	float reci = 1.0f / (b[uAxis] * c[vAxis] - b[vAxis] * c[uAxis]);         
	triAccel.b_nu = b[uAxis] * reci;         triAccel.b_nv = -b[vAxis] * reci;         
	triAccel.b_d = (b[vAxis] * tri.mVertPos[0][uAxis] - b[uAxis] * tri.mVertPos[0][vAxis]) * reci;         
	
	// second line equation         
	triAccel.c_nu = c[uAxis] * -reci;         
	triAccel.c_nv = c[vAxis] * reci;         
	triAccel.c_d = (c[vAxis] * tri.mVertPos[0][uAxis] - c[uAxis] * tri.mVertPos[0][vAxis]) * -reci; 

	// triangle id
	triAccel.tri_id = tri_id;

	// calculate the max edge length
	float le0 = nvmath::lengthSquared(tri.mVertPos[0] - tri.mVertPos[1]);
	float le1 = nvmath::lengthSquared(tri.mVertPos[0] - tri.mVertPos[2]);
	float le2 = nvmath::lengthSquared(tri.mVertPos[2] - tri.mVertPos[1]);
	float maxl = (le0 > le1 ? le0 : le1);
	maxl = (maxl > le2 ? maxl : le2);
	triAccel.E_F = sqrtf(maxl);
	if (normal[triAccel.k] < 0)
		triAccel.E_F = -triAccel.E_F;

}

bool RayIntersect(const KRay& ray, const KAccelTriangleOpt& tri, IntersectContext& ctx)
{
	if (intersect_one_ray(tri, ray, ctx)) {
		ctx.w = 1.0f - ctx.u - ctx.v;
		ctx.tri_id = tri.tri_id;
		return true;
	}
	return false;

}


