#include "eye_ray_gen.h"
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

	KVec3d offsetVec = mViewUp * mTanFovY;
	mTopVec = mViewDir + offsetVec;
	mBottomVec = mViewDir - offsetVec;
	offsetVec = mHorizonVec * mTanFovX;
	mLeftVec = mViewDir - offsetVec;
	mRightVec = mViewDir + offsetVec;
}

void EyeRayGen::SetFov(double xfov)
{
	mTanFovX = xfov;
}

void EyeRayGen::GenEyeVector(KVec3d& eye_vec, double x, double y) const
{
	KVec3 tempVec;
	tempVec = nvmath::lerp(x / mWidth, mLeftVec, mRightVec);
	tempVec -= mViewDir;
	eye_vec = nvmath::lerp(y / mHeight, mBottomVec, mTopVec);
	eye_vec += tempVec;
}

void EyeRayGen::GenerateEyeRayFocal(double x, double y, KVec3d& outFocalPt) const
{
	KVec3d eyeVec;
	GenEyeVector(eyeVec, x, y);
	double s = 1.0 / (eyeVec * mViewDir);
	eyeVec *= (mFocalPlaneDis * s);

	outFocalPt = mEyePos + eyeVec;
}

double EyeRayGen::GetPixelCoverageRatio(const KVec3d& ray_dir) const
{
	double res = ray_dir *mViewDir;
	res *= (mTanFovX / mWidth);
	return res;
}

bool EyeRayGen::GetImageCoordidates(const KVec3& pos, KVec2& out_image_pos) const
{
	KVec3d vdir = ToVec3d(pos) - mEyePos;
	vdir.normalize();
	double sx = vdir * mHorizonVec;
	double sy = vdir * mViewUp;
	double sz = vdir * mViewDir;

	double cx = sqrt(1.0f - fabs(sx)*fabs(sx));
	double cy = sqrt(1.0f - fabs(sy)*fabs(sy));
	if (sz < 0)
		return false;

	out_image_pos[0] = float(sx / (mTanFovX * sz));
	out_image_pos[1] = float(sy / (mTanFovY * sz));
	return true;
}
