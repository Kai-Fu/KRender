#pragma once
#include "../base/BaseHeader.h"

namespace Sampling {

void Halton2D(KVec2* result, int start, int n, const KVec2& warp, const KVec2& min, const KVec2& max, int p2 = 3);

}