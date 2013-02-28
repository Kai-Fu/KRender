#pragma once
#include <string>
#include <set>

class UniqueStringMaker
{
public:
	UniqueStringMaker();
	~UniqueStringMaker();

	void MakeUniqueString(std::string& outStr, const char* pSrc);
	void Clear();
private:
	std::set<std::string> mUniqueStr;
};
