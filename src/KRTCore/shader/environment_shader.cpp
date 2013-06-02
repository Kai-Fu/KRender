#include "environment_shader.h"

KEnvShader* KEnvShader::s_currentEvnShader = NULL;

KEnvShader::KEnvShader()
{

}

KEnvShader::~KEnvShader()
{

}

const KEnvShader* KEnvShader::GetEnvShader()
{
	return s_currentEvnShader;
}

void KEnvShader::UseSpheeEnvironment(const KColor& upClr, const KColor& downClr, float transHeight)
{
	if (s_currentEvnShader)
		delete s_currentEvnShader;
	
	s_currentEvnShader = new KHemishpereEnvShader(upClr, downClr, transHeight);
}

void KEnvShader::UseCubemapEnvironment(const char* filename)
{
	if (s_currentEvnShader)
		delete s_currentEvnShader;
	
	s_currentEvnShader = new KCubeEvnShader(filename);
}

KHemishpereEnvShader::KHemishpereEnvShader(const KColor& upClr, const KColor& downClr, float transHeight)
{
	mColorUp = upClr;
	mColorDown = downClr;
	mTransitionHeight = transHeight;
}

KHemishpereEnvShader::~KHemishpereEnvShader()
{

}

void KHemishpereEnvShader::Sample(const KVec3& pos, const KVec3& dir, KColor& outClr) const
{
	KVec3 nDir = dir;
	nDir.normalize();
	if (nDir[2] > fabs(mTransitionHeight)) {
		if (nDir[2] > 0)
			outClr = mColorUp;
		else
			outClr = mColorDown;
	}
	else {
		float n = fabs(mTransitionHeight);
		float l = (nDir[2] + n) / n * 0.5f;
		outClr = mColorDown;
		outClr.Lerp(mColorUp, l);
	}
}

KCubeEvnShader::KCubeEvnShader(const char* filename)
{
	mCubeMap.SetSourceFile(filename);
}

KCubeEvnShader::~KCubeEvnShader()
{

}

void KCubeEvnShader::Sample(const KVec3& pos, const KVec3& dir, KColor& outClr) const
{
	KVec4 res = mCubeMap.SampleBilinear(dir);
	outClr.r = res[0];
	outClr.g = res[1];
	outClr.b = res[2];
}