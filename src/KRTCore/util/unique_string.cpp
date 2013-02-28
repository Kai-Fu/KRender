#include "unique_string.h"
#include "HelperFunc.h"

UniqueStringMaker::UniqueStringMaker()
{

}

UniqueStringMaker::~UniqueStringMaker()
{

}

void UniqueStringMaker::MakeUniqueString(std::string& outStr, const char* pSrc)
{
	std::string ret;
	int iter = 0;
	char iterStr[12];
	ret = pSrc;
	if (mUniqueStr.find(ret) != mUniqueStr.end()) {
		do {
			ret = pSrc;
			IntToStr(iter, iterStr, 10);
			ret += iterStr;
			++iter;
		} while (mUniqueStr.find(ret) != mUniqueStr.end());
	}

	mUniqueStr.insert(ret);
	outStr = ret;
}

void UniqueStringMaker::Clear()
{
	mUniqueStr.clear();
}
