#pragma once
#include "../base/base_header.h"
#include "../base/geometry.h"

class EyeRayGen
{
public:
	EyeRayGen(void);
	virtual ~EyeRayGen(void);

	void SetImageSize(UINT32 w, UINT32 h);
	void SetFov(double xfov);
	void GenerateEyeRayFocal(double x, double y, KVec3d& outFocalPt) const;
	double GetPixelCoverageRatio(const KVec3d& ray_dir) const;
	bool GetImageCoordidates(const KVec3& pos, KVec2& out_image_pos) const;

protected:
	void GenEyeVector(KVec3d& vec, double x, double y) const;
public:
	KVec3d mEyePos;
	KVec3d mViewDir;
	KVec3d mViewUp;
	KVec3d mHorizonVec;
	double mTanFovX;
	double mTanFovY;
	double mWidth;
	double mHeight;
	double mFocalPlaneDis;
	KVec3d mPixelOffset;

private:
	// Vectors used internally to generate eye rays
	KVec3d mLeftVec;
	KVec3d mRightVec;
	KVec3d mTopVec;
	KVec3d mBottomVec;

};
