#pragma once
#include "../base/BaseHeader.h"
#include "../base/geometry.h"

namespace KAnimation {

/** This class is to handle rigid body animation, it will take a bounding box and
	a sequence of translation, rotation and scaling, then its responsibility is to interpolate the transform
	at a given time and compute the total bounding box that will overlap the object during 
	the animation.
**/
class LocalTRSFrame {
public:
	struct LclTRS {
		KMatrix4 node_tm;
		KMatrix4 inv_node_tm;
		nvmath::Quatf node_rot;
	};

	void Reset(const KMatrix4& single_trans);
	void Reset(const KMatrix4& starting, const KMatrix4& ending);
	void ComputeTotalBBox(const KBBox& in_box, KBBox& out_box) const;
	void Interpolate(float cur_t, LclTRS& out_TRS) const;
	static void TransformRay(KRay& out_ray, const KRay& in_ray, const LclTRS& trs);
	static void Decompose(LclTRS& out_trs, const KMatrix4& mat);

private:
	bool mIsMoving;
	LclTRS mStartingFrame;
	LclTRS mEndingFrame;
};

}