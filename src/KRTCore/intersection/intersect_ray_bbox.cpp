#include "intersect_ray_bbox.h"
#include <tmmintrin.h>

bool IntersectBBox(const KRay& ray, const KBBoxOpt& bbox, float& t0, float& t1);
// Ray-box intersection using IEEE numerical properties to ensure that the
// test is both robust and efficient, as described in:
//
//      Amy Williams, Steve Barrus, R. Keith Morley, and Peter Shirley
//      "An Efficient and Robust Ray-Box Intersection Algorithm"
//      Journal of graphics tools, 10(1):49-54, 2005
//		You can get an intersection point by grabbing tmin (if return is true). 
bool IntersectBBox(const KRay& ray, const KBBox& bbox, float& t0, float& t1)
{
	float tmin = (bbox[ray.mSign[0]][0] - ray.GetOrg()[0]) * ray.mRcpDir[0];
	float tmax = (bbox[1-ray.mSign[0]][0] - ray.GetOrg()[0]) * ray.mRcpDir[0];

	float tymin = (bbox[ray.mSign[1]][1] - ray.GetOrg()[1]) * ray.mRcpDir[1];
	float tymax = (bbox[1-ray.mSign[1]][1] - ray.GetOrg()[1]) * ray.mRcpDir[1];

	if (tymin > tmin)
		tmin = tymin;
	if (tymax < tmax)
		tmax = tymax;

	float tzmin = (bbox[ray.mSign[2]][2] - ray.GetOrg()[2]) * ray.mRcpDir[2];
	float tzmax = (bbox[1-ray.mSign[2]][2] - ray.GetOrg()[2]) * ray.mRcpDir[2];

	if (tzmin > tmin)
		tmin = tzmin;
	if (tzmax < tmax)
		tmax = tzmax;
	// [Added by Kai] Output the near & far intersection point(if they exist)
	t0 = tmin;
	t1 = tmax;
	return (t1 >= t0);
}

bool IntersectBBox(const KRay& ray, const KBBoxOpt& bbox, float& t0, float& t1)
{
	//float tmin = (bbox[ray.mSign[0]][0] - ray.GetOrg()[0]) * ray.mRcpDir[0];
	//float tmax = (bbox[1-ray.mSign[0]][0] - ray.GetOrg()[0]) * ray.mRcpDir[0];

	//float tymin = (bbox[ray.mSign[1]][1] - ray.GetOrg()[1]) * ray.mRcpDir[1];
	//float tymax = (bbox[1-ray.mSign[1]][1] - ray.GetOrg()[1]) * ray.mRcpDir[1];
	vec4 xxyy_shuffled;
	vec4 t_min_max;
	vec4 temp1;
	xxyy_shuffled.asUINT4 = _mm_shuffle_epi8(bbox.mXXYY.asUINT4, ray.mOrign_shuffle_0011);
	vec4f temp0 = _mm_sub_ps(xxyy_shuffled.asFloat4, ray.mOrign_0011);
	t_min_max.asFloat4 = _mm_mul_ps(temp0, ray.mRcpDir_0011);
	temp1.asUINT4 = _mm_shuffle_epi32(t_min_max.asUINT4, _MM_SHUFFLE(3,2,3,2));
	vec4f t_min = _mm_max_ps(t_min_max.asFloat4, temp1.asFloat4);
	vec4f t_max = _mm_min_ps(t_min_max.asFloat4, temp1.asFloat4);

	/*if (tymin > tmin)
		tmin = tymin;
	if (tymax < tmax)
		tmax = tymax;*/

	/*float tzmin = (bbox[ray.mSign[2]][2] - ray.GetOrg()[2]) * ray.mRcpDir[2];
	float tzmax = (bbox[1-ray.mSign[2]][2] - ray.GetOrg()[2]) * ray.mRcpDir[2];*/
	vec4 zz_shuffled;
	zz_shuffled.asUINT4 = _mm_shuffle_epi8(bbox.mZZ.asUINT4, ray.mOrign_shuffle_22);
	temp0 = _mm_sub_ps(zz_shuffled.asFloat4, ray.mOrign_22);
	temp1.asFloat4 = _mm_mul_ps(temp0, ray.mRcpDir_22);

	/*if (tzmin > tmin)
		tmin = tzmin;
	if (tzmax < tmax)
		tmax = tzmax;*/
	t_min = _mm_max_ps(t_min, temp1.asFloat4);
	t_max = _mm_min_ps(t_max, temp1.asFloat4);
	// [Added by Kai] Output the near & far intersection point(if they exist)
	t0 = vec4_f(t_min, 0);
	t1 = vec4_f(t_max, 1);

	return (t1 >= t0);
}

vec4i IntersectBBox(const KRay& ray, const KBBox4& bbox, vec4f& t0, vec4f& t1)
{
	//float tmin = (bbox[ray.mSign[0]][0] - ray.GetOrg()[0]) * ray.mRcpDir[0];
	vec4 res;
	res.asFloat4 = vec4_zero();
	vec4f temp_vec0, temp_vec1, temp_vec2;
	temp_vec0 = vec4_splats(ray.mRcpDir[0]);
	temp_vec1 = vec4_splats(ray.GetOrg()[0]);
	temp_vec2 = vec4_sub(bbox[ray.mSign[0]][0], temp_vec1);
	vec4f tmin = vec4_mul(temp_vec2, temp_vec0);

	//float tmax = (bbox[1-ray.mSign[0]][0] - ray.GetOrg()[0]) * ray.mRcpDir[0];
	temp_vec2 = vec4_sub(bbox[1 - ray.mSign[0]][0], temp_vec1);
	vec4f tmax = vec4_mul(temp_vec2, temp_vec0);

	//float tymin = (bbox[ray.mSign[1]][1] - ray.GetOrg()[1]) * ray.mRcpDir[1];
	temp_vec0 = vec4_splats(ray.mRcpDir[1]);
	temp_vec1 = vec4_splats(ray.GetOrg()[1]);
	temp_vec2 = vec4_sub(bbox[ray.mSign[1]][1], temp_vec1);
	vec4f tymin = vec4_mul(temp_vec2, temp_vec0);

	//float tymax = (bbox[1-ray.mSign[1]][1] - ray.GetOrg()[1]) * ray.mRcpDir[1];
	temp_vec2 = vec4_sub(bbox[1 - ray.mSign[1]][1], temp_vec1);
	vec4f tymax = vec4_mul(temp_vec2, temp_vec0);

	//if ( (tmin > tymax) || (tymin > tmax) )
		//return false;
	vec4f mask0 = vec4_cmple(tmin, tymax);
	vec4f mask1 = vec4_cmple(tymin, tmax);
	vec4f mask = vec4_and(mask0, mask1);
	if (!vec4_gather(mask)) return res.asUINT4;
	/*if (tymin > tmin)
		tmin = tymin;
	if (tymax < tmax)
		tmax = tymax;*/
	tmin = vec4_sel(tmin, tymin, vec4_cmpgt(tymin, tmin));
	tmax = vec4_sel(tmax, tymax, vec4_cmpgt(tmax, tymax));

	//float tzmin = (bbox[ray.mSign[2]][2] - ray.GetOrg()[2]) * ray.mRcpDir[2];
	temp_vec0 = vec4_splats(ray.mRcpDir[2]);
	temp_vec1 = vec4_splats(ray.GetOrg()[2]);
	temp_vec2 = vec4_sub(bbox[ray.mSign[2]][2], temp_vec1);
	vec4f tzmin = vec4_mul(temp_vec2, temp_vec0);

	//float tzmax = (bbox[1-ray.mSign[2]][2] - ray.GetOrg()[2]) * ray.mRcpDir[2];
	temp_vec2 = vec4_sub(bbox[1 - ray.mSign[2]][2], temp_vec1);
	vec4f tzmax = vec4_mul(temp_vec2, temp_vec0);

	//if ( (tmin > tzmax) || (tzmin > tmax) )
		//return false;
	mask0 = vec4_cmple(tmin, tzmax);
	mask1 = vec4_cmple(tzmin, tmax);
	mask = vec4_and(mask0, mask1);
	//if (!vec4_gather(mask)) return res.asUINT4;

	/*if (tzmin > tmin)
		tmin = tzmin;
	if (tzmax < tmax)
		tmax = tzmax;*/
	tmin = vec4_sel(tmin, tzmin, vec4_cmpgt(tzmin, tmin));
	tmax = vec4_sel(tmax, tzmax, vec4_cmpgt(tmax, tzmax));
	// [Added by Kai] Output the near & far intersection point(if they exist)
	t0 = tmin;
	t1 = tmax;

	res.asFloat4 = vec4_cmpgt(tmax, tmin);
	return res.asUINT4;

}
