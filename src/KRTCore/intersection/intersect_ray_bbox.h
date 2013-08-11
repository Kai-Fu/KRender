#pragma once

#include "../base/geometry.h"

bool IntersectBBox(const KRay& ray, const KBBox& bbox, float& t0, float& t1);
