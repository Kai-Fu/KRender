#include "camera.h"
#include "../file_io/file_io_template.h"
#include <assert.h>

KCamera::KCamera() :
	mApertureSize(-1.0f, -1.0f),
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

const KVec2& KCamera::GetApertureSize() const
{
	return mApertureSize;
}

void KCamera::SetApertureSize(float x, float y)
{
	mApertureSize[0] = x;
	mApertureSize[1] = y;
}

void KCamera::SetupStillCamera(const MotionState& param)
{
	mMotionStates.clear();
	mMotionStates.push_back(param);
}

void KCamera::SetupMovingCamera()
{
	mMotionStates.clear();
}

void KCamera::AddCameraMotionState(const MotionState& param)
{
	mMotionStates.push_back(param);
}

static void _CalcTimeInterpParams(float t, UINT32 ms_len, UINT32& out_ms_start, float& lerp_t)
{
	assert(ms_len > 1);
	float l = (t + 0.5f) * 0.5f * float(ms_len - 1);
	int il = int(l);
	lerp_t = l - (float)il;

	if (il >= (int)ms_len) {
		il = int(ms_len - 2);
		lerp_t = 1.0f;
	}

	if (il < 0) {
		il = 0;
		lerp_t = 0.0f;
	}

	out_ms_start = (UINT32)il;
}

void KCamera::InterpolateCameraMotion(MotionState& outMotion, float t) const
{
	assert(mMotionStates.size() > 0);

	if (mMotionStates.size() > 1) {
		UINT32 out_ms_start;
		float lerp_t;
		_CalcTimeInterpParams(t, (UINT32)mMotionStates.size(), out_ms_start, lerp_t);
		const MotionState& ms0 = mMotionStates[out_ms_start];
		const MotionState& ms1 = mMotionStates[out_ms_start + 1];
		outMotion.pos = nvmath::lerp(lerp_t, ms0.pos, ms1.pos);
		outMotion.lookat = nvmath::lerp(lerp_t, ms0.lookat, ms1.lookat);
		outMotion.xfov = nvmath::lerp(lerp_t, ms0.xfov, ms1.xfov);
		outMotion.focal = nvmath::lerp(lerp_t, ms0.focal, ms1.focal);
	}
	else {
		outMotion = mMotionStates[0];
	}
	
}

void KCamera::ConfigEyeRayGen(EyeRayGen& outEyeRayGen, MotionState& outMotion, float t) const
{
	InterpolateCameraMotion(outMotion, t);

	KVec3 viewVec = outMotion.lookat - outMotion.pos;
	viewVec.normalize();

	KVec3 horVec;
	horVec = viewVec ^ outMotion.up;
	horVec.normalize();

	outEyeRayGen.mViewUp = horVec ^ viewVec;
	outEyeRayGen.mViewDir = viewVec;
	outEyeRayGen.mHorizonVec = horVec;
	outEyeRayGen.SetFov(tanf(outMotion.xfov * nvmath::PI / 180.0f * 0.5f));
	outEyeRayGen.mEyePos = outMotion.pos;
	outEyeRayGen.mFocalPlaneDis = outMotion.focal;

	outEyeRayGen.SetImageSize(mImageWidth, mImageHeight);
}


bool KCamera::Save(FILE* pFile)
{
	std::string typeName = "KCamera";
	if (!SaveArrayToFile(typeName, pFile))
		return false;

	if (!SaveArrayToFile(mMotionStates, pFile))
		return false;

	if (!SaveTypeToFile(mApertureSize, pFile))
		return false;

	return true;
}

bool KCamera::Load(FILE* pFile)
{
	std::string srcTypeName;
	std::string dstTypeName = "KCamera"; 
	if (!LoadArrayFromFile(srcTypeName, pFile))
		return false;
	if (srcTypeName.compare(dstTypeName) != 0)
		return false;

	if (!LoadArrayFromFile(mMotionStates, pFile))
		return false;

	if (!LoadTypeFromFile(mApertureSize, pFile))
		return false;

	return true;
}
