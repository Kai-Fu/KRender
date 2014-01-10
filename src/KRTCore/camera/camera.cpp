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

KVec2 KCamera::GetApertureSize(double cur_t) const
{
	KVec2 ret;
	if (mIsMoving) {
		ret = nvmath::lerp((float)cur_t, mStatringState.aperture, mEndingState.aperture);
	}
	else {
		ret = mStatringState.aperture;
	}
	return ret;
}


void KCamera::SetupStillCamera(const MotionState& param)
{
	mIsMoving = false;
	mStatringState = param;
}

void KCamera::SetupMovingCamera(const MotionState& starting, const MotionState& ending)
{
	mIsMoving = true;
	mStatringState = starting;
	mEndingState = ending;
}


void KCamera::InterpolateCameraMotion(MotionState& outMotion, double cur_t) const
{
	if (mIsMoving) {
		outMotion.pos = nvmath::lerp(cur_t, mStatringState.pos, mEndingState.pos);
		outMotion.lookat = nvmath::lerp(cur_t, mStatringState.lookat, mEndingState.lookat);
		outMotion.xfov = nvmath::lerp(cur_t, mStatringState.xfov, mEndingState.xfov);
		outMotion.focal = nvmath::lerp(cur_t, mStatringState.focal, mEndingState.focal);
	}
	else {
		outMotion = mStatringState;
	}
	
}

void KCamera::ConfigEyeRayGen(EyeRayGen& outEyeRayGen, MotionState& outMotion, double cur_t) const
{
	InterpolateCameraMotion(outMotion, cur_t);

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

