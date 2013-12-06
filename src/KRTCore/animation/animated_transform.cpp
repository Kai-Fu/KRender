#include "animated_transform.h"
#include "../file_io/file_io_template.h"
#include <common/math/Trafo.h>

namespace KAnimation {

void LocalTRSFrame::Reset(const KMatrix4& single_trans)
{
	mIsMoving = false;

	mStartingFrame.trs.setMatrix(single_trans);
	mEndingFrame.trs = mStartingFrame.trs;
}

void LocalTRSFrame::Reset(const KMatrix4& starting, const KMatrix4& ending)
{
	mIsMoving = true;

	mStartingFrame.trs.setMatrix(starting);
	mEndingFrame.trs.setMatrix(ending);

	if (starting == ending)
		mIsMoving = false;
}


void LocalTRSFrame::ComputeTotalBBox(const KBBox& in_box, KBBox& out_box) const
{
	out_box.SetEmpty();

	KBBox frameBBox(in_box);
	frameBBox.TransformByMatrix(mStartingFrame.trs.getMatrix());
	out_box.Add(frameBBox);

	if (mIsMoving) {
		KBBox frameBBox(in_box);
		frameBBox.TransformByMatrix(mEndingFrame.trs.getMatrix());
		out_box.Add(frameBBox);
	}
}

void LocalTRSFrame::Interpolate(float cur_t, LocalTRSFrame::LclTRS& out_TRS) const
{
	if (mIsMoving) {

		out_TRS.trs = nvmath::lerp(cur_t, mStartingFrame.trs, mEndingFrame.trs);
	}
	else {
		out_TRS = mStartingFrame;
	}
}



void LocalTRSFrame::TransformRay(KRay& out_ray, const KRay& in_ray, const LocalTRSFrame::LclTRS& trs)
{
	KMatrix4 inv_trs = trs.trs.getInverse();
	KVec3d newOrig;
	Vec3TransformCoord(newOrig, in_ray.GetOrg(), inv_trs);
	KVec3d oldDst = in_ray.GetOrg() + in_ray.GetDir();
	KVec3d newDst;
	Vec3TransformCoord(newDst, oldDst, inv_trs);
	KVec3d newDir = newDst - newOrig;

	out_ray.Init(newOrig, newDir, NULL);
}

}