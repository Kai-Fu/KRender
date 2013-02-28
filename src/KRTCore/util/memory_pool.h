#pragma once
#include "../os/api_wrapper.h"
#include <vector>


// This class provides fast memory allocation, it assume the memory is not freed when performing operation
class GlowableMemPool
{
public:
	GlowableMemPool(size_t blockSize = 8192);
	~GlowableMemPool();

	void* Alloc(size_t s);
	void Reset();
private:
	std::vector<void*> mMemBlocks;
	SPIN_LOCK_FLAG mAllocCS;
	size_t mCurAllocSize;
	size_t mBlockSize;
};

// This class provides thread-safe operation for adding and accessing, not removing operation is provided
template <typename T, int glowSize>
class SharedArray
{
private:
	T* mData;
	volatile long mCnt;
	volatile long mReservedCnt;
	SPIN_LOCK_FLAG mResizeLock;

public:
	SharedArray() {mData = NULL; mCnt = 0; mReservedCnt = 0; mResizeLock = 0;}
	~SharedArray() {};

	void Append(const T* pData)
	{
		if (mReservedCnt < mCnt) {
			EnterSpinLockCriticalSection_Share(mResizeLock);

			long old_idx = atomic_increment(&mCnt) - 1;
			memcpy(mData + old_idx, pData, sizeof(T));

			LeaveSpinLockCriticalSection_Share(mResizeLock);
		}
		else {
			EnterSpinLockCriticalSection(mResizeLock);
			
			mReservedCnt += glowSize;
			mData = (T*)realloc(mData, mReservedCnt);
			memcpy(mData + mCnt, pData, sizeof(T));
			++mCnt;

			LeaveSpinLockCriticalSection(mResizeLock);
		}
	}

	template <typename F>
	UINT32 ForEach(F& func) const
	{
		volatile long* pLock = const_cast<volatile long*>(&mResizeLock);
		EnterSpinLockCriticalSection_Share(*pLock);

		long ret = mCnt;
		for (long i = 0; i < mCnt; ++i) {
			func(mData[i]);
		}
		
		LeaveSpinLockCriticalSection_Share(*pLock);
		return ret;
	}

};