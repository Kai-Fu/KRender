#pragma once

#include "../base/BaseHeader.h"
#include "../base/geometry.h"

#include <string>



float Rand_0_1();
float Rand_1_1();

class KTimer
{
public:
	KTimer(bool bStartImm);

	void Start();
	double Stop();

private:
	double mStartTime;
};


void GetPathDir(const char* path, std::string& out_dir);
void IntToStr(int val, char* buf, int base);


float ComputeWeightAndIndex(UINT32 frameCnt, float cur_t, UINT32& floorIdx, UINT32& ceilingIdx);

void SwizzleForSIMD(void* srcData, void* destData, int SIMD_cc, int SIMD_cw, int elemSize, int elemCount);