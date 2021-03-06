#include "image_filter.h"
#include "../os/api_wrapper.h"
#include "../entry/constants.h"
#include <assert.h>

KImageFilterBase::KImageFilterBase(const BitmapObject* bitmap, UINT32 threadCnt)
{
	mpBitmapObj = bitmap;
	mFilterTasks.resize(threadCnt);
	for (UINT32 i = 0; i < threadCnt; ++i) {
		mFilterTasks[i].mpParent = this;
	}
}

void KImageFilterBase::FilterTask::Execute()
{
	Tile2DSet::TileDesc desc;
	while (mpParent->mTile2D.GetNextTile(desc)) {
		mpParent->DoFilter(desc.start_x, desc.start_y, desc.tile_w, desc.tile_h);
	}
}

KImageFilterBase::~KImageFilterBase()
{

}

bool KImageFilterBase::RunFilter(ThreadModel::ThreadBucket* pThreadBucket)
{
	if (!mpBitmapObj)
		return false;

	mTile2D.Reset(mpBitmapObj->mWidth, mpBitmapObj->mHeight, 64);
	assert(pThreadBucket->GetThreadCnt() == mFilterTasks.size());
	for (UINT32 i = 0; i < (UINT32)mFilterTasks.size(); ++i)
		pThreadBucket->SetThreadTask(i, &mFilterTasks[i]);
	
	return pThreadBucket->Run();
}

KRBG32F_EdgeDetecter::KRBG32F_EdgeDetecter(const BitmapObject* bitmap, UINT32 threadCnt) : KImageFilterBase(bitmap, threadCnt)
{
	mWritingLocker = 0;
	mResult.Resize(bitmap->mWidth * bitmap->mHeight);
	mResult.ClearAll();
}

void KRBG32F_EdgeDetecter::DoFilter(UINT32 sx, UINT32 sy, UINT32 w, UINT32 h)
{
	for (UINT32 xx = 0; xx < w-1; ++xx) {
		for (UINT32 yy = 0; yy < h-1; ++yy) {
			KColor clr0 = mpBitmapObj->GetPixel(sx + xx, sy + yy).color;
			KColor clr1 = mpBitmapObj->GetPixel(sx + xx + 1, sy + yy).color;
			KColor clr2 = mpBitmapObj->GetPixel(sx + xx, sy + yy + 1).color;
			KColor clr3 = mpBitmapObj->GetPixel(sx + xx + 1, sy + yy + 1).color;
			KColor sum;
			sum = clr0;
			sum.Add(clr1);
			sum.Add(clr2);
			sum.Add(clr3);
			sum.Scale(0.25f);

			if (clr0.DiffRatio(sum) > COLOR_DIFF_THRESH_HOLD ||
				clr1.DiffRatio(sum) > COLOR_DIFF_THRESH_HOLD ||
				clr2.DiffRatio(sum) > COLOR_DIFF_THRESH_HOLD ||
				clr3.DiffRatio(sum) > COLOR_DIFF_THRESH_HOLD) {
				
				EnterSpinLockCriticalSection(mWritingLocker);
				mResult.Set(sx + xx + (sy + yy) *  mpBitmapObj->mWidth);
				mResult.Set(sx + xx + 1 + (sy + yy) *  mpBitmapObj->mWidth);
				mResult.Set(sx + xx + (sy + yy + 1) *  mpBitmapObj->mWidth);
				mResult.Set(sx + xx + 1 + (sy + yy + 1) *  mpBitmapObj->mWidth);
				LeaveSpinLockCriticalSection(mWritingLocker);

			}

		}
	}
	
}

bool KRBG32F_EdgeDetecter::IsEdge(UINT32 x, UINT32 y) const
{
	return mResult.IsSet(x + y *  mpBitmapObj->mWidth);
}