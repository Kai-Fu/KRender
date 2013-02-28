#pragma once
#include "../base/BaseHeader.h"
#include "../base/geometry.h"

class EyeRayGen
{
public:
	EyeRayGen(void);
	virtual ~EyeRayGen(void);

	void SetImageSize(UINT32 w, UINT32 h);
	void SetFov(float xfov);
	void GenerateEyeRayFocal(float x, float y, KVec3& outFocalPt) const;
	float GetPixelCoverageRatio(const KVec3& ray_dir) const;
	bool GetImageCoordidates(const KVec3& pos, KVec2& out_image_pos) const;

	bool SaveView(FILE* pFile);
	bool LoadView(FILE* pFile);
protected:
	void GenEyeVector(KVec3& vec, float x, float y) const;
public:
	KVec3 mEyePos;
	KVec3 mViewDir;
	KVec3 mViewUp;
	KVec3 mHorizonVec;
	float mTanFovX;
	float mTanFovY;
	float mWidth;
	float mHeight;
	float mFocalPlaneDis;
	KVec3 mPixelOffset;

private:
	// Vectors used internally to generate eye rays
	KVec3 mLeftVec;
	KVec3 mRightVec;
	KVec3 mTopVec;
	KVec3 mBottomVec;

};
