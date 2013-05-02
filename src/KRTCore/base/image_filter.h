#pragma once
#include "../image/BitmapObject.h"
#include "../util/thread_model.h"
#include "../util/tile2d.h"
#include "../base/bit_array.h"
#include "../os/api_wrapper.h"

class KImageFilterBase
{
public:
	KImageFilterBase(const BitmapObject* bitmap, UINT32 threadCnt);
	~KImageFilterBase();

	virtual void DoFilter(UINT32 sx, UINT32 sy, UINT32 w, UINT32 h) = 0;

	class FilterTask : public ThreadModel::IThreadTask
	{
	public:
		KImageFilterBase* mpParent;
		
		virtual void Execute();
	};
	bool RunFilter(ThreadModel::ThreadBucket* pThreadBucket);

protected:
	const BitmapObject* mpBitmapObj;

private:
	Tile2DSet mTile2D;
	std::vector<FilterTask> mFilterTasks;
};

class KRBG32F_EdgeDetecter : public KImageFilterBase
{
public:
	KRBG32F_EdgeDetecter(const BitmapObject* bitmap, UINT32 threadCnt);
	virtual void DoFilter(UINT32 sx, UINT32 sy, UINT32 w, UINT32 h);

	bool IsEdge(UINT32 x, UINT32 y) const;
private:
	SPIN_LOCK_FLAG mWritingLocker;
	KBitArray mResult;
};