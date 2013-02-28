#include "animated_transform.h"
#include "../file_io/file_io_template.h"
#include <common/math/Trafo.h>

namespace KAnimation {

void LocalTRSFrame::Reset()
{
	mKeyFrameTrans.clear();
}

void LocalTRSFrame::AddKeyFrame(const KMatrix4& mat)
{
	LclTRS trs;
	trs.inv_node_tm = mat;
	trs.node_tm = mat;
	trs.inv_node_tm.invert();

	nvmath::Trafo trans_info;
	trans_info.setMatrix(mat);
	trs.node_rot = trans_info.getOrientation();

	mKeyFrameTrans.push_back(trs);
}

void LocalTRSFrame::ComputeTotalBBox(const KBBox& in_box, KBBox& out_box) const
{
	out_box.SetEmpty();
	if (mKeyFrameTrans.size() > 0) {
		for (unsigned int i = 0; i < mKeyFrameTrans.size(); ++i) {
			KBBox frameBBox(in_box);
			frameBBox.TransformByMatrix(mKeyFrameTrans[i].node_tm);
			out_box.Add(frameBBox);
		}
	}
	else
		out_box = in_box;
}

void LocalTRSFrame::Interpolate(float t, LocalTRSFrame::LclTRS& out_TRS) const
{
	if (mKeyFrameTrans.size() > 1) {
		float interval = 1.0f / (float)(mKeyFrameTrans.size() - 1);
		float slot = t / interval;
		int i_slot = (int)slot;
		float delta = slot - (float)i_slot;

		out_TRS.inv_node_tm = nvmath::lerp(delta, mKeyFrameTrans[i_slot].inv_node_tm, mKeyFrameTrans[i_slot + 1].inv_node_tm);
		out_TRS.node_tm = nvmath::lerp(delta, mKeyFrameTrans[i_slot].node_tm, mKeyFrameTrans[i_slot + 1].node_tm);
		out_TRS.node_rot = nvmath::lerp(delta, mKeyFrameTrans[i_slot].node_rot, mKeyFrameTrans[i_slot + 1].node_rot);
	}
	else if (mKeyFrameTrans.size() == 1)  {
		out_TRS = mKeyFrameTrans[0];
	}
	else {
		out_TRS.node_tm = nvmath::cIdentity44f;
		out_TRS.inv_node_tm = nvmath::cIdentity44f;
		out_TRS.node_rot = KQuat(0,0,0,1);
	}
}


bool LocalTRSFrame::Save(FILE* pFile) const
{
	if (!SaveArrayToFile(mKeyFrameTrans, pFile))
		return false;
	return true;
}

bool LocalTRSFrame::Load(FILE* pFile)
{
	if (!LoadArrayFromFile(mKeyFrameTrans, pFile))
		return false;
	return true;
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