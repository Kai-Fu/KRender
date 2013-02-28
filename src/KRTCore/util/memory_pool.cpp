#include "memory_pool.h"

GlowableMemPool::GlowableMemPool(size_t blockSize)
{
	mBlockSize = blockSize;
	mAllocCS = 0;
	mCurAllocSize = mBlockSize + 1;
}

GlowableMemPool::~GlowableMemPool()
{
	Reset();
}

void* GlowableMemPool::Alloc(size_t s)
{
	void* pRet = NULL;
	EnterSpinLockCriticalSection(mAllocCS);
	size_t pLast = 0;
	if (!mMemBlocks.empty())
		pLast = (size_t)mMemBlocks[mMemBlocks.size() - 1] + mCurAllocSize;
	mCurAllocSize += s;
	if (mCurAllocSize < mBlockSize) 
		pRet = (void*)pLast;
	else {
		void* pNew = malloc(s > mBlockSize ? s : mBlockSize);
		if (pNew) {
			mMemBlocks.push_back(pNew);
			pRet = pNew;
			mCurAllocSize = s;
		}
	}
	LeaveSpinLockCriticalSection(mAllocCS);
	return pRet;
}

void GlowableMemPool::Reset()
{
	for (size_t i = 0; i < mMemBlocks.size(); ++i)
		free(mMemBlocks[i]);
	mMemBlocks.clear();
	mCurAllocSize = mBlockSize + 1;
}