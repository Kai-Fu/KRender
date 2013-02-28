#pragma once
#include <vector>
#include <common/defines/typedefs.h>

class KBitArray
{
public:
	KBitArray();
	KBitArray(size_t len);

	void Resize(size_t size);
	bool IsSet(size_t idx) const;
	void Set(size_t idx);
	void Clear(size_t idx);
	void ClearAll();

private:
	std::vector<UINT32> mData;
};