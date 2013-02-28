#pragma once

#include "../base/geometry.h"   

bool RayIntersect(const KRay& ray, const KAccleTriVertPos& tri, UINT32 tri_id, IntersectContext& ctx);
bool RayIntersect(const KRay& ray, const KAccelTriangleOpt& tri, IntersectContext& ctx);
bool RayIntersect4Tri(const KRay& ray, const KAccelTriangleOpt1r4t* tri, UINT32 tri4_cnt, IntersectContext& ctx);


#if RAY_TRI_METHOD0
/* the original jgt code */
int intersect_triangle(const float* orig, const float* dir,
		       const float* vert0, const float* vert1, const float* vert2,
		       float *t, float *u, float *v);
#elif RAY_TRI_METHOD1
/* code rewritten to do tests on the sign of the determinant */
/* the division is at the end in the code                    */
int intersect_triangle(const float* orig, const float* dir,
		       const float* vert0, const float* vert1, const float* vert2,
		       float *t, float *u, float *v);
#elif RAY_TRI_METHOD3
/* code rewritten to do tests on the sign of the determinant */
/* the division is before the test of the sign of the det    */
int intersect_triangle(const float* orig, const float* dir,
		       const float* vert0, const float* vert1, const float* vert2,
		       float *t, float *u, float *v);
#elif RAY_TRI_METHOD4
/* code rewritten to do tests on the sign of the determinant */
/* the division is before the test of the sign of the det    */
/* and one CROSS has been moved out from the if-else if-else */
int intersect_triangle(const float* orig, const float* dir,
		       const float* vert0, const float* vert1, const float* vert2,
		       float *t, float *u, float *v);

#endif
