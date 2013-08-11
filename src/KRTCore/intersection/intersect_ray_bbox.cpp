#include "intersect_ray_bbox.h"
#include <tmmintrin.h>

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

