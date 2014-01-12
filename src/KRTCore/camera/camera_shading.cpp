#include "camera.h"
#include "../entry/TracingThread.h"
#include "../shader/shader_api.h"
#include "../shader/surface_shader.h"


bool KCamera::EvaluateShading(TracingInstance& tracingInstance, KColor& out_clr)
{
	EvalContext& evalCtx = tracingInstance.mCameraContext;
	MotionState ts;
	ConfigEyeRayGen(evalCtx.mEyeRayGen, ts, evalCtx.inMotionTime);

	KVec3d eyePos = 
		evalCtx.mEyeRayGen.mHorizonVec * evalCtx.inAperturePos[0] * ts.aperture[0] + 
		evalCtx.mEyeRayGen.mViewUp * evalCtx.inAperturePos[1] * ts.aperture[1];

	eyePos += ts.pos;
	KVec3d eyeLookAt;
	evalCtx.mEyeRayGen.GenerateEyeRayFocal(evalCtx.inScreenPos[0], evalCtx.inScreenPos[1], eyeLookAt);
	KRay ray;
	ray.Init(eyePos, eyeLookAt - eyePos, NULL);

	return CalcuShadingByRay(&tracingInstance, ray, out_clr, NULL);
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