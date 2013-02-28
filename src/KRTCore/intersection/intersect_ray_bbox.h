#pragma once

#include "../base/geometry.h"

bool IntersectBBox(const KRay& ray, const KBBox& bbox, float& t0, float& t1);
bool IntersectBBox(const KRay& ray, const KBBoxOpt& bbox, float& t0, float& t1);

vec4i IntersectBBox(const KRay& ray, const KBBox4& bbox, vec4f& t0, vec4f& t1);