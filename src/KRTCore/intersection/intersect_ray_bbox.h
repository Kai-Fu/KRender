#pragma once

#include "../base/geometry.h"

bool IntersectBBox(const KRay& ray, const KBBox& bbox, double& t0, double& t1);
