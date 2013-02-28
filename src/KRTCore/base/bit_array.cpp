#include "bit_array.h"

KBitArray::KBitArray()
{

}

KBitArray::KBitArray(size_t len)
{
	Resize(len);
}

void KBitArray::Resize(size_t size)
{
	mData.resize(size / 32 + 1);
}

bool KBitArray::IsSet(size_t idx) const
{
	size_t ridx = idx / 32;
	size_t bidx = idx % 32;
	return (mData[ridx] >> bidx) & 0x00000001;
}

void KBitArray::Set(size_t idx)
{
	size_t ridx = idx / 32;
	size_t bidx = idx % 32;
	mData[ridx] |= (0x00000001 << bidx);
}

void KBitArray::Clear(size_t idx)
{
	size_t ridx = idx / 32;
	size_t bidx = idx % 32;
	mData[ridx] &= ~(0x00000001 << bidx);
}

void KBitArray::ClearAll()
{
	for (size_t i = 0; i < mData.size(); ++i) {
		mData[i] = 0;
	}
}