#include "EyeRayGen.h"
#include "../file_io/file_io_template.h"

EyeRayGen::EyeRayGen(void)
{
	mFocalPlaneDis = 2.0f;
	SetImageSize(1, 1);
	mPixelOffset[0] = 0;
	mPixelOffset[1] = 0;
}

EyeRayGen::~EyeRayGen(void)
{
}

void EyeRayGen::SetImageSize(UINT32 w, UINT32 h)
{
	mWidth	= (float)w;
	mHeight	= (float)h;
	mTanFovY = mTanFovX * (float)mHeight / (float)mWidth;

	KVec3 offsetVec = mViewUp * mTanFovY;
	mTopVec = mViewDir + offsetVec;
	mBottomVec = mViewDir - offsetVec;
	offsetVec = mHorizonVec * mTanFovX;
	mLeftVec = mViewDir - offsetVec;
	mRightVec = mViewDir + offsetVec;
}

void EyeRayGen::SetFov(float xfov)
{
	mTanFovX = xfov;
}

void EyeRayGen::GenEyeVector(KVec3& eye_vec, float x, float y) const
{
	KVec3 tempVec;
	tempVec = nvmath::lerp(x / mWidth, mLeftVec, mRightVec);
	tempVec -= mViewDir;
	eye_vec = nvmath::lerp(y / mHeight, mBottomVec, mTopVec);
	eye_vec += tempVec;
}

void EyeRayGen::GenerateEyeRayFocal(float x, float y, KVec3& outFocalPt) const
{
	KVec3 eyeVec;
	GenEyeVector(eyeVec, x, y);
	float s = 1.0f / (eyeVec * mViewDir);
	eyeVec *= (mFocalPlaneDis * s);

	outFocalPt = mEyePos + eyeVec;
}

float EyeRayGen::GetPixelCoverageRatio(const KVec3& ray_dir) const
{
	float res = ray_dir *mViewDir;
	res *= (mTanFovX / mWidth);
	return res;
}

bool EyeRayGen::GetImageCoordidates(const KVec3& pos, KVec2& out_image_pos) const
{
	KVec3 vdir = pos - mEyePos;
	vdir.normalize();
	float sx = vdir * mHorizonVec;
	float sy = vdir * mViewUp;
	float sz = vdir * mViewDir;

	float cx = sqrt(1.0f - fabs(sx)*fabs(sx));
	float cy = sqrt(1.0f - fabs(sy)*fabs(sy));
	if (sz < 0)
		return false;

	out_image_pos[0] = sx / (mTanFovX * sz);
	out_image_pos[1] = sy / (mTanFovY * sz);
	return true;
}

bool EyeRayGen::SaveView(FILE* pFile)
{
	if (!SaveTypeToFile(mEyePos, pFile))
		return false;
	if (!SaveTypeToFile(mViewDir, pFile))
		return false;
	if (!SaveTypeToFile(mViewUp, pFile))
		return false;

	if (!SaveTypeToFile(mTanFovX, pFile))
		return false;

	return true;
}

bool EyeRayGen::LoadView(FILE* pFile)
{
	if (!LoadTypeFromFile(mEyePos, pFile))
		return false;
	if (!LoadTypeFromFile(mViewDir, pFile))
		return false;
	if (!LoadTypeFromFile(mViewUp, pFile))
		return false;
	mHorizonVec = mViewDir ^ mViewUp;

	if (!LoadTypeFromFile(mTanFovX, pFile))
		return false;

	return true;
}