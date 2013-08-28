#include "geometry.h"
#include <assert.h>
#include "../os/api_wrapper.h"

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

void KBBox::TransformByMatrix(const KMatrix4d& mat)
{
	KBBox newBBox;
	for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < 2; ++j) {
			KVec3d v = ToVec3d((*this)[j]);
			v[i] = (*this)[j == 1 ? 0 : 1][i];
			KVec3d out_v;
			Vec3TransformCoord(out_v, v, mat);
			newBBox.ContainVert(ToVec3f(out_v));
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
			ContainVert(tri.mVertPos[i] + tri.mVertPos_Delta[i]);
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

void KBoxNormalizer::InitFromBBox(const KBBox& box)
{
	mCenter = box.Center();
	KVec3 scale = box.mMax - box.mMin;
	mRcpScale[0] = 1.0f / scale[0];
	mRcpScale[1] = 1.0f / scale[1];
	mRcpScale[2] = 1.0f / scale[2];
	mNormRcpScale = mRcpScale;
	mRcpScaleLen = mNormRcpScale.normalize();
}

void KBoxNormalizer::ApplyToMatrix(KMatrix4& mat) const
{
	KMatrix4 tmpMat;
	nvmath::setTransMat(tmpMat, -mCenter);
	mat *= tmpMat;
	nvmath::setMat(tmpMat, KVec3(0,0,0), mRcpScale);
	mat *= tmpMat;
}

void KBoxNormalizer::ApplyToRay(KVec3& rayOrg, KVec3& rayDir) const
{
	rayOrg -= mCenter;

	rayOrg[0] *= mRcpScale[0];
	rayOrg[1] *= mRcpScale[1];
	rayOrg[2] *= mRcpScale[2];

	rayDir[0] *= mNormRcpScale[0];
	rayDir[1] *= mNormRcpScale[1];
	rayDir[2] *= mNormRcpScale[2];
}

void Vec3TransformCoord(KVec3& res, const KVec3& v, const KMatrix4& mat)
{
	KVec4 temp = KVec4(v, 1.0f) * mat;

	res[0] = temp[0] / temp[3];
	res[1] = temp[1] / temp[3];
	res[2] = temp[2] / temp[3];
}

void Vec3TransformCoord(KVec3d& res, const KVec3d& v, const KMatrix4d& mat)
{
	KVec4d temp = KVec4d(v, 1.0f) * mat;

	res[0] = temp[0] / temp[3];
	res[1] = temp[1] / temp[3];
	res[2] = temp[2] / temp[3];
}

void Vec3TransformCoord(KVec3d& res, const KVec3d& v, const KMatrix4& mat)
{
	for (unsigned int i = 0; i < 3; ++i)
		res[i] = v[0]*mat[0][i] + v[1]*mat[1][i] + v[2]*mat[2][i] + mat[3][i]; 

	double r4 = v[0]*mat[0][3] + v[1]*mat[1][3] + v[2]*mat[2][3] + mat[3][3]; 
	res /= r4;
}

void Vec3TransformNormal(KVec3& res, const KVec3& v, const KMatrix4& mat)
{
	KVec3 orig, dst;
	Vec3TransformCoord(orig, KVec3(0,0,0), mat);
	Vec3TransformCoord(dst, v, mat);
	res = dst - orig;
	res.normalize();
}

KMemCacher::KMemCacher(UINT32 bucketSize, UINT32 alignment)
{
	if (bucketSize < 10)
		bucketSize = 10;

	mEntries.resize(bucketSize);
	mAllocAlignment = alignment;
	for (size_t i = 0; i < mEntries.size(); ++i) {
		mEntries[i].pMem = NULL;
	}

	Reset();
}

KMemCacher::~KMemCacher()
{
	Reset();
}

void KMemCacher::Reset()
{
	mLRU_TimeStamp = 0;
	for (size_t i = 0; i < mEntries.size(); ++i) {
		mEntries[i].id = INVALID_ENTRY_IDX;
		if (mEntries[i].pMem)
			Aligned_Free(mEntries[i].pMem);
		mEntries[i].pMem = NULL;
		mEntries[i].memSize = 0;
		mEntries[i].lru_flag = 0;
	}
}

bool KMemCacher::EntryExists(UINT64 id)
{
	UINT32 hashedIdx = id % (UINT32)mEntries.size();
	for (size_t i = 0; i < mEntries.size(); ++i) {
		UINT32 curIdx = (hashedIdx + i) % (UINT32)mEntries.size();
		if (mEntries[curIdx].id == id)
			return true;
	}

	return false;
}

void* KMemCacher::LRU_OpenEntry(UINT64 id,  UINT32 memSize)
{
	UINT32 targetIdx = INVALID_INDEX;
	UINT32 hashedIdx = id % (UINT32)mEntries.size();
	UINT64 oldestEntryTS = INVALID_ENTRY_IDX;
	UINT32 oldestEntryIdx = INVALID_INDEX;
	++mLRU_TimeStamp;
	for (size_t i = 0; i < mEntries.size(); ++i) {
		UINT32 curIdx = (hashedIdx + i) % (UINT32)mEntries.size();
		if (mEntries[curIdx].id == id || mEntries[curIdx].id == INVALID_ENTRY_IDX) {
			targetIdx = curIdx;
			break;
		}

		if (oldestEntryTS < mEntries[curIdx].lru_flag) {
			oldestEntryTS = mEntries[curIdx].lru_flag;
			oldestEntryIdx = curIdx;
		}

	}

	if (targetIdx == INVALID_INDEX) {
		targetIdx = oldestEntryIdx;
	}

	mEntries[targetIdx].lru_flag = mLRU_TimeStamp;
	if (mEntries[targetIdx].pMem == NULL || mEntries[targetIdx].memSize < memSize) {
		mEntries[targetIdx].pMem = Aligned_Realloc(mEntries[targetIdx].pMem, memSize, mAllocAlignment);
		mEntries[targetIdx].memSize = memSize;
	}

	return mEntries[targetIdx].pMem;
}

