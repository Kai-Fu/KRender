#pragma once

#include "../base/geometry.h"   

bool RayIntersect(const float* ray_org, const float* ray_dir, const KTriVertPos2& tri, float cur_t, RayTriIntersect& out_info);


