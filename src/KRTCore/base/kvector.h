#pragma once
#include <stdio.h>
#include <assert.h>
#include "../os/api_wrapper.h"

typedef size_t KVEC_SIZE;


#include "../os/api_wrapper.h"

template <typename T>
class KVectorA16
{
private:
	T* mData;
	KVEC_SIZE mAllocSize;
	KVEC_SIZE mSize;
	static const KVEC_SIZE BLOCK_SIZE = 4096;
public:
	KVectorA16() {
		assert((sizeof(T) % 16) == 0);
		mData = NULL;
		mAllocSize = 0;
		resize(0);
	}

	KVectorA16(KVEC_SIZE s) {
		assert((sizeof(T) % 16) == 0);
		mData = NULL;
		mAllocSize = 0;
		resize(s);
	}

	~KVectorA16() {
		clear();
	}

	void reserve(KVEC_SIZE c) {
		if (c > mAllocSize) {
			KVEC_SIZE newAllocSize = ((c + BLOCK_SIZE - 1) / BLOCK_SIZE) * BLOCK_SIZE;
			mData = (T*)Aligned_Realloc(mData, newAllocSize*sizeof(T), 16);
			mAllocSize = newAllocSize;
		}
	}

	T* Append() {
		reserve(mSize + 1);

		T* pAppended = mData + mSize;
		++mSize;
		return pAppended;
	}

	void Append(const T& data) {
		reserve(mSize + 1);

		T* pAppended = mData + mSize;
		++mSize;

		*pAppended = data;
	}

	void AppendRawData(const void* pData, KVEC_SIZE s) {
		assert((s % sizeof(T)) == 0);
		KVEC_SIZE cnt = s / sizeof(T);
		reserve(mSize + cnt);
		T* pAppended = mData + mSize;
		mSize += cnt;

		memcpy(pAppended, pData, s);
	}

	T& operator [] (KVEC_SIZE idx) {
		assert(idx < mSize);
		T* pRet = mData + idx;
		return *pRet;
	}

	const T& operator [] (KVEC_SIZE idx) const {
		assert(idx < mSize);
		T* pRet = mData + idx;
		return *pRet;
	}

	void resize(KVEC_SIZE s) {
		reserve(s);
		mSize = s;
	}

	KVEC_SIZE size() const {
		return mSize;
	}

	bool empty() const {
		return (mSize == 0);
	}

	void clear() {
		if (mData)
			Aligned_Free(mData);
		mData = NULL;
		mAllocSize = 0;
		mSize = 0;
	}

};
