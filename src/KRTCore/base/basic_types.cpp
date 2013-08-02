#include "geometry.h"
#include <assert.h>

KBSphere::KBSphere()
{
	SetEmpty();
}

KBSphere::KBSphere(const KBSphere& ref)
{
	mCenter = ref.mCenter;
	mRadius = ref.mRadius;
}

void KBSphere::SetEmpty()
{
	mCenter[0] = mCenter[1] = mCenter[2] = 0;
	mRadius = 0;
}

bool KBSphere::IsEmpty() const
{
	return (mRadius == 0);
}

bool KBSphere::IsInside(const KVec3& vert) const
{
	KVec3 dis = vert - mCenter;
	return (nvmath::lengthSquared(dis) <= (mRadius*mRadius));
}

bool KBBox::IsOverlapping(const KBSphere& sphere) const
{
	float distance = 0;
	for (int i=0; i<3; ++i) {
		if (sphere.mCenter[i] < mMin[i]) {
			float d = sphere.mCenter[i]-mMin[i];
			distance += d*d;
		} else if (sphere.mCenter[i] > mMax[i]) {
			float d = sphere.mCenter[i]-mMax[i];
			distance += d*d;
		}
	}
	return (distance < sphere.mRadius*sphere.mRadius);
}

KBBox::KBBox()
{
	SetEmpty();
}

void KBBox::SetEmpty()
{
	mMin = KVec3(FLT_MAX, FLT_MAX, FLT_MAX);
	mMax = KVec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
}

void KBBox::Enlarge(float factor)
{
	KVec3 c = ((mMin + mMax) * 0.5f);
	KVec3 d = mMax - c;
	d *= factor;
	mMax = c + d;
	mMin = c - d;
}

void KBBox::Add(const KBBox& bbox)
{
	mMin[0] = mMin[0] < bbox.mMin[0] ? mMin[0] : bbox.mMin[0];
	mMin[1] = mMin[1] < bbox.mMin[1] ? mMin[1] : bbox.mMin[1];
	mMin[2] = mMin[2] < bbox.mMin[2] ? mMin[2] : bbox.mMin[2];

	mMax[0] = mMax[0] > bbox.mMax[0] ? mMax[0] : bbox.mMax[0];
	mMax[1] = mMax[1] > bbox.mMax[1] ? mMax[1] : bbox.mMax[1];
	mMax[2] = mMax[2] > bbox.mMax[2] ? mMax[2] : bbox.mMax[2];
}

void KBBox::ContainVert(const KVec3& vert)
{
	mMin[0] = mMin[0] < vert[0] ? mMin[0] : vert[0];
	mMin[1] = mMin[1] < vert[1] ? mMin[1] : vert[1];
	mMin[2] = mMin[2] < vert[2] ? mMin[2] : vert[2];

	mMax[0] = mMax[0] > vert[0] ? mMax[0] : vert[0];
	mMax[1] = mMax[1] > vert[1] ? mMax[1] : vert[1];
	mMax[2] = mMax[2] > vert[2] ? mMax[2] : vert[2];
}

void KBBox::ClampBBox(const KBBox& bbox)
{
	mMin[0] = mMin[0] < bbox.mMin[0] ? bbox.mMin[0] : mMin[0];
	mMin[1] = mMin[1] < bbox.mMin[1] ? bbox.mMin[1] : mMin[1];
	mMin[2] = mMin[2] < bbox.mMin[2] ? bbox.mMin[2] : mMin[2];

	mMax[0] = mMax[0] > bbox.mMax[0] ? bbox.mMax[0] : mMax[0];
	mMax[1] = mMax[1] > bbox.mMax[1] ? bbox.mMax[1] : mMax[1];
	mMax[2] = mMax[2] > bbox.mMax[2] ? bbox.mMax[2] : mMax[2];
}

void KBBox::TransformByMatrix(const KMatrix4& mat)
{
	KBBox newBBox;
	for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < 2; ++j) {
			KVec3 v = (*this)[j];
			v[i] = (*this)[j == 1 ? 0 : 1][i];
			KVec3 out_v;
			Vec3TransformCoord(out_v, v, mat);
			newBBox.ContainVert(out_v);
		}
	}
	(*this) = newBBox;
}

KBBox::KBBox(const KTriVertPos2& tri)
{
	SetEmpty();
	for (int i = 0; i < 3; ++i)
		ContainVert(tri.mVertPos[i]);

	if (tri.mIsMoving) {
		for (int i = 0; i < 3; ++i)
		ContainVert(tri.mVertPos_Ending[i]);
	}

	KVec3 diagnal(mMax - mMin);
	float len = nvmath::length(diagnal);
	// We need to be carefully to tweak this value to avoid precision issue
	float epsilon = len * 0.0001f;
	for (int i = 0; i < 3; ++i) {
		if (fabs(mMax[i] - mMin[i]) < epsilon) {
			mMin[i] -= 2*epsilon;
			mMax[i] += 2*epsilon;
			break;
		}
	}
}

bool KBBox::IsEmpty() const
{
	return (mMax[0] < mMin[0] ||
		mMax[1] < mMin[1] ||
		mMax[2] < mMin[2]);
}

bool KBBox::IsInside(const KVec3& vert) const
{
	if (vert[0] < mMin[0] ||
		vert[0] > mMax[0] ||
		vert[1] < mMin[1] ||
		vert[1] > mMax[1] ||
		vert[2] < mMin[2] ||
		vert[2] > mMax[2])
		return false;
	else
		return true;
}

const KVec3 KBBox::Center() const
{
	return ((mMin + mMax) * 0.5f);
}

void KBBox::GetFaceArea(float area[3]) const
{
	KVec3 len = mMax - mMin;
	area[0] = fabs(len[1] * len[2]);
	area[1] = fabs(len[0] * len[2]);
	area[2] = fabs(len[0] * len[1]);
}

void KBBox4::FromBBox(const KBBox* bbox, UINT32 cnt)
{
	for (UINT32 ki = 0; ki < 4; ++ki) {

		UINT32 bi = ki > (cnt-1) ? (cnt-1) : ki;
		for (int i = 0; i < 3; ++i) {
			vec4_f(mMin[i], ki) = bbox[bi].mMin[i];
			vec4_f(mMax[i], ki) = bbox[bi].mMax[i];
		}
	}
}

const KBBox& KBBox::operator = (const KBBoxOpt& bbox)
{
	mMax[0] = vec4_f(bbox.mXXYY, 0);
	mMin[0] = vec4_f(bbox.mXXYY, 1);
	mMax[1] = vec4_f(bbox.mXXYY, 2);
	mMin[1] = vec4_f(bbox.mXXYY, 3);
	mMax[2] = vec4_f(bbox.mZZ, 0);
	mMin[2] = vec4_f(bbox.mZZ, 1);
	mMax[2] = vec4_f(bbox.mZZ, 2);
	mMin[2] = vec4_f(bbox.mZZ, 3);
	return *this;
}

KBBoxOpt::KBBoxOpt()
{

}

KBBoxOpt::KBBoxOpt(const KBBox& bbox)
{
	*this = bbox;
}

const KBBoxOpt& KBBoxOpt::operator =(const KBBox& bbox)
{
	vec4_f(mXXYY, 0) = bbox.mMax[0];
	vec4_f(mXXYY, 1) = bbox.mMin[0];
	vec4_f(mXXYY, 2) = bbox.mMax[1];
	vec4_f(mXXYY, 3) = bbox.mMin[1];
	vec4_f(mZZ, 0) = bbox.mMax[2];
	vec4_f(mZZ, 1) = bbox.mMin[2];
	vec4_f(mZZ, 2) = bbox.mMax[2];
	vec4_f(mZZ, 3) = bbox.mMin[2];
	return *this;
}

void Vec3TransformCoord(KVec3& res, const KVec3& v, const KMatrix4& mat)
{
	KVec4 temp = KVec4(v, 1.0f) * mat;

	res[0] = temp[0] / temp[3];
	res[1] = temp[1] / temp[3];
	res[2] = temp[2] / temp[3];
}

void Vec3TransformNormal(KVec3& res, const KVec3& v, const KMatrix4& mat)
{
	KVec3 orig, dst;
	Vec3TransformCoord(orig, KVec3(0,0,0), mat);
	Vec3TransformCoord(dst, v, mat);
	res = dst - orig;
	res.normalize();
}

