#include "HelperFunc.h"
#include "../os/api_wrapper.h"
#include <string>
#include <vector>
#include <assert.h>

float Rand_0_1()
{
	return (float)rand() / (float)RAND_MAX;
}

float Rand_1_1()
{
	return (float)rand() / (float)RAND_MAX * 2.0f - 1.0f;
}

KTimer::KTimer(bool bStartImm)
{
	if (bStartImm)
		Start();

}

void KTimer::Start()
{
	mStartTime = GetSystemElapsedTime();
}

double KTimer::Stop()
{
	return GetSystemElapsedTime() - mStartTime;
}

void GetPathDir(const char* path, std::string& out_dir)
{
	std::string dir;

	dir = path;
	size_t tIdx = 0;
	const char* lastSlash = strrchr(path, '/');
	if (lastSlash) {
		tIdx = lastSlash - path;
	}
	else {
		lastSlash = strrchr(path, '\\');
		if (lastSlash) 
			tIdx = lastSlash - path;
	}

	if (dir.length() > 0)
		dir[tIdx] = '\0';
	
	out_dir = dir;

}

void IntToStr(int val, char* buf, int base)
{
	int i = 0;
	for(; val; ++i, val /= base)
		buf[i] = "0123456789abcdef"[val % base];

	buf[i] = '\0';

	for (int j = 0; j < i/2; ++j) 
		std::swap(buf[j], buf[i-j-1]);
	
}


float ComputeWeightAndIndex(UINT32 frameCnt, float cur_t, UINT32& floorIdx, UINT32& ceilingIdx)
{
	assert(cur_t >= 0 && cur_t <= 1.0f);
	float fIdx = float(frameCnt - 1) * cur_t;
	float fFloor = floor(fIdx);
	floorIdx = (UINT32)fFloor;
	ceilingIdx = (UINT32)ceil(fIdx);
	assert(floorIdx < frameCnt);
	if (ceilingIdx >= frameCnt) {
		ceilingIdx = frameCnt - 1;
		return 0;
	}
	else {
		return fIdx - fFloor;
	}
}

// |	float	|	float	|	float	|	float	|
// |	SIMD_cw	|	SIMD_cw	|	SIMD_cw |	SIMD_cw	|
//			SIMD_cc = 4
//
void SwizzleForSIMD(void* srcData, void* destData, int SIMD_cc, int SIMD_cw, int elemSize, int elemCount)
{
	int SIMD_size = SIMD_cc * SIMD_cw;
	int SIMD_dataCount = elemSize * elemCount / SIMD_size;

	assert(elemSize % SIMD_cw == 0);
	int paddingElemCount = elemCount;
	int lowerPaddingElemCount = elemCount;
	if (elemCount % SIMD_cc != 0) {
		lowerPaddingElemCount -= (elemCount % SIMD_cc);
		paddingElemCount = lowerPaddingElemCount + SIMD_cc;
	}
	
	for (int elem_i = 0; elem_i < elemCount; ++elem_i) {
		BYTE* curSrcElem = (BYTE*)srcData + elem_i * elemSize;
		BYTE* curDestElem = (BYTE*)destData + (elem_i / SIMD_cc) * elemSize * SIMD_cc + (elem_i % SIMD_cc) * SIMD_cw;
		for (int elem_di = 0; elem_di < elemSize; elem_di += SIMD_cw) {
			BYTE* curSrcSIMD_c = curSrcElem + elem_di;
			BYTE* curDestSIMD_c = curDestElem + elem_di * SIMD_cc;

			memcpy(curDestSIMD_c, curSrcSIMD_c, SIMD_cw);
		}
	}

	int lastElemIdx = elemCount - 1;
	for (int elem_i = elemCount; elem_i < paddingElemCount; ++elem_i) {
		BYTE* curSrcElem = (BYTE*)srcData + lastElemIdx * elemSize;
		BYTE* curDestElem = (BYTE*)destData + (elem_i / SIMD_cc) * elemSize * SIMD_cc + (elem_i % SIMD_cc) * SIMD_cw;
		for (int elem_di = 0; elem_di < elemSize; elem_di += SIMD_cw) {
			BYTE* curSrcSIMD_c = curSrcElem + elem_di;
			BYTE* curDestSIMD_c = curDestElem + elem_di * SIMD_cc;

			memcpy(curDestSIMD_c, curSrcSIMD_c, SIMD_cw);
		}
	}
}