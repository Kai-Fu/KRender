#pragma once

#include "../base/BaseHeader.h"
#include "../base/geometry.h"

#include <string>



float Rand_0_1();
float Rand_1_1();

class KRT_API KTimer
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
