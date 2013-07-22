#include "animated_transform.h"
#include "../file_io/file_io_template.h"
#include <common/math/Trafo.h>

namespace KAnimation {

void LocalTRSFrame::Reset(const KMatrix4& single_trans)
{
	mIsMoving = false;

	mStartingFrame.inv_node_tm = single_trans;
	mStartingFrame.node_tm = single_trans;
	mStartingFrame.inv_node_tm.invert();
	nvmath::Trafo trans_info;
	trans_info.setMatrix(single_trans);
	mStartingFrame.node_rot = trans_info.getOrientation();
}

void LocalTRSFrame::Reset(const KMatrix4& starting, const KMatrix4& ending)
{
	mIsMoving = true;

	mStartingFrame.inv_node_tm = starting;
	mStartingFrame.node_tm = starting;
	mStartingFrame.inv_node_tm.invert();
	nvmath::Trafo trans_info;
	trans_info.setMatrix(starting);
	mStartingFrame.node_rot = trans_info.getOrientation();

	mEndingFrame.inv_node_tm = ending;
	mEndingFrame.node_tm = ending;
	mEndingFrame.inv_node_tm.invert();
	trans_info.setMatrix(ending);
	mEndingFrame.node_rot = trans_info.getOrientation();

	if (starting == ending)
		mIsMoving = false;
}


void LocalTRSFrame::ComputeTotalBBox(const KBBox& in_box, KBBox& out_box) const
{
	out_box.SetEmpty();

	KBBox frameBBox(in_box);
	frameBBox.TransformByMatrix(mStartingFrame.node_tm);
	out_box.Add(frameBBox);

	if (mIsMoving) {
		KBBox frameBBox(in_box);
		frameBBox.TransformByMatrix(mEndingFrame.node_tm);
		out_box.Add(frameBBox);
	}
}

void LocalTRSFrame::Interpolate(float cur_t, LocalTRSFrame::LclTRS& out_TRS) const
{
	if (mIsMoving) {

		out_TRS.inv_node_tm = nvmath::lerp(cur_t, mStartingFrame.inv_node_tm, mEndingFrame.inv_node_tm);
		out_TRS.node_tm = nvmath::lerp(cur_t, mStartingFrame.node_tm, mEndingFrame.node_tm);
		out_TRS.node_rot = nvmath::lerp(cur_t, mStartingFrame.node_rot, mEndingFrame.node_rot);
	}
	else {
		out_TRS = mStartingFrame;
	}
}



void LocalTRSFrame::TransformRay(KRay& out_ray, const KRay& in_ray, const LocalTRSFrame::LclTRS& trs)
{
	KVec3 newOrig;
	Vec3TransformCoord(newOrig, in_ray.GetOrg(), trs.inv_node_tm);
	KVec3 oldDst = in_ray.GetOrg() + in_ray.GetDir();
	KVec3 newDst;
	Vec3TransformCoord(newDst, oldDst, trs.inv_node_tm);
	KVec3 newDir = newDst - newOrig;

	out_ray.Init(newOrig, newDir, NULL);
}

}