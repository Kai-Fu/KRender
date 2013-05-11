#include "HelperFunc.h"
#include "../os/api_wrapper.h"
#include <string>
#include <vector>

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
