#include "camera.h"
#include "../file_io/file_io_template.h"
#include <assert.h>

KCamera::KCamera() :
	mIsMoving(false),
	mImageWidth(0),
	mImageHeight(0)
{

}

KCamera::~KCamera()
{

}

void KCamera::SetImageSize(UINT32 w, UINT32 h)
{
	mImageWidth = w;
	mImageHeight = h;
}


void KCamera::SetupStillCamera(const MotionState& param)
{
	mIsMoving = false;
	mStartingState = param;
}

void KCamera::SetupMovingCamera(const MotionState& starting, const MotionState& ending)
{
	mIsMoving = true;
	mStartingState = starting;
	mEndingState = ending;
}


void KCamera::InterpolateCameraMotion(MotionState& outMotion, const MotionState& ms0, const MotionState& ms1, double cur_t)
{
	outMotion.pos = nvmath::lerp(cur_t, ms0.pos, ms1.pos);
	outMotion.lookat = nvmath::lerp(cur_t, ms0.lookat, ms1.lookat);
	outMotion.up = nvmath::lerp(cur_t, ms0.up, ms1.up);
	outMotion.xfov = nvmath::lerp(cur_t, ms0.xfov, ms1.xfov);
	outMotion.focal = nvmath::lerp(cur_t, ms0.focal, ms1.focal);
	outMotion.aperture = nvmath::lerp(cur_t, ms0.aperture, ms1.aperture);
}

void KCamera::ConfigEyeRayGen(EyeRayGen& outEyeRayGen, MotionState& outMotion, double cur_t) const
{
	if (mIsMoving)
		InterpolateCameraMotion(outMotion, mStartingState, mEndingState, cur_t);
	else
		outMotion = mStartingState;

	KVec3d viewVec = outMotion.lookat - outMotion.pos;
	viewVec.normalize();

	KVec3d horVec;
	horVec = viewVec ^ outMotion.up;
	horVec.normalize();

	outEyeRayGen.mViewUp = horVec ^ viewVec;
	outEyeRayGen.mViewDir = viewVec;
	outEyeRayGen.mHorizonVec = horVec;
	outEyeRayGen.SetFov(tan(outMotion.xfov * nvmath::PI / 180.0 * 0.5));
	outEyeRayGen.mEyePos = outMotion.pos;
	outEyeRayGen.mFocalPlaneDis = outMotion.focal;

	outEyeRayGen.SetImageSize(mImageWidth, mImageHeight);
}

