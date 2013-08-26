#pragma once

#include "../base/geometry.h"   

bool RayIntersect(const float* ray_org, const float* ray_dir, const KTriVertPos2& tri, float cur_t, RayTriIntersect& out_info);

void RayIntersectStaticTriArray(const float* ray_org, const float* ray_dir, const float* tri_pos, float* tuv, unsigned int cnt);
void RayIntersectAnimTriArray(const float* ray_org, const float* ray_dir, float cur_t, const float* tri_pos, float* tuv, unsigned int cnt);