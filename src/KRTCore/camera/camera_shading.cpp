#include "camera.h"
#include "../entry/TracingThread.h"
#include "../shader/surface_shader_api.h"
#include "../shader/surface_shader.h"


bool KCamera::EvaluateShading(TracingInstance& tracingInstance, KColor& out_clr)
{
	EvalContext& evalCtx = tracingInstance.mCameraContext;
	MotionState ts;
	ConfigEyeRayGen(evalCtx.mEyeRayGen, ts, evalCtx.inMotionTime);

	KVec3 eyePos = evalCtx.mEyeRayGen.mHorizonVec * evalCtx.inAperturePos[0] + evalCtx.mEyeRayGen.mViewUp * evalCtx.inAperturePos[1];
	eyePos += ts.pos;
	KVec3 eyeLookAt;
	evalCtx.mEyeRayGen.GenerateEyeRayFocal(evalCtx.inScreenPos[0], evalCtx.inScreenPos[1], eyeLookAt);
	KRay ray;
	ray.Init(eyePos, eyeLookAt - eyePos, NULL);

	return CalcuShadingByRay(&tracingInstance, ray, out_clr, NULL);
}

void KCamera::CastRay(TracingInstance& tracingInstance, float x, float y, const KVec2& aperturePos, KRay& outRay) const
{
	const KBBox& sceneBBox = tracingInstance.GetScenePtr()->GetSceneBBox();
	MotionState ts;
	EyeRayGen eyeGen;
	ConfigEyeRayGen(eyeGen, ts, tracingInstance.mCameraContext.inMotionTime);

	float theta = aperturePos[0] * 2.0f * nvmath::PI;
	float t0 = sinf(theta) * aperturePos[1];
	float t1 = cosf(theta) * aperturePos[1];
	KVec3 aperture_x = eyeGen.mHorizonVec * (mApertureSize[0] * 0.5f);
	KVec3 aperture_y = eyeGen.mViewUp * (mApertureSize[1] * 0.5f);

	KVec3 adjustedOrg = aperture_x * t0 + aperture_y * t1;
	KVec3 rayOrg(eyeGen.mEyePos), rayTarget;
	eyeGen.GenerateEyeRayFocal(x, y, rayTarget);
	rayOrg += adjustedOrg;
	KVec3 rayDir = rayTarget - rayOrg;
	rayDir.normalize();
	outRay.Init(eyeGen.mEyePos, rayDir, &sceneBBox);
}

bool KCamera::GetScreenPosition(const KVec3& pos, KVec2& outScrPos) const
{
	MotionState ts;
	EyeRayGen eyeGen;
	ConfigEyeRayGen(eyeGen, ts, 0);
	KVec2 res;
	if (eyeGen.GetImageCoordidates(pos, res)) {
		outScrPos = res * 0.5f + KVec2(0.5f, 0.5f);
		outScrPos[0] *= (float)mImageWidth;
		outScrPos[1] *= (float)mImageHeight ;
		return true;
	}
	else
		return false;
}