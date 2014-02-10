#pragma once
#include "../base/base_header.h"

// The algorithm used in Hammersley spherical sampling is from "Sampling with Hammersley and Halton Points" by Tien-Tsin Wong
namespace Sampling {

void HammersleySphere(KVec3* result, int start, int n);

}