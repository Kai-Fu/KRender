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

 bool KEnvShader::Initialize()
 {
	 //UseSphereEnvironment(KColor(0,0,0.9f), KColor(0.2f, 0.2f, 0.2f), 0.8f);
	 return SetEnvironmentShader("./asset/environment/default.template");
 }

 void KEnvShader::Shutdown()
 {
	 if (s_currentEvnShader)
		delete s_currentEvnShader;
	 s_currentEvnShader = NULL;
 }

void KEnvShader::UseSphereEnvironment(const KColor& upClr, const KColor& downClr, float transHeight)
{
	if (s_currentEvnShader)
		delete s_currentEvnShader;
	
	s_currentEvnShader = new KHemishpereEnvShader(upClr, downClr, transHeight);
}

bool KEnvShader::SetEnvironmentShader(const char* templateFile)
{
	if (s_currentEvnShader)
		delete s_currentEvnShader;

	KSC_EnvShader* newEvnShader = new KSC_EnvShader(templateFile);
	if (newEvnShader->IsValid()) {
		s_currentEvnShader = newEvnShader;
		return true;
	}
	else {
		delete newEvnShader;
		return false;
	}
}


KHemishpereEnvShader::KHemishpereEnvShader(const KColor& upClr, const KColor& downClr, float transHeight)
{
	mColorUp = upClr;
	mColorDown = downClr;
	mTransitionHeight = fabs(transHeight);
}

KHemishpereEnvShader::~KHemishpereEnvShader()
{

}

void KHemishpereEnvShader::Sample(const KVec3& pos, const KVec3& dir, KColor& outClr) const
{
	KVec3 nDir = dir;
	nDir.normalize();
	float majorN = nDir[1];
	if (majorN >= mTransitionHeight)
		outClr = mColorUp;
	else if (majorN < -mTransitionHeight)
		outClr = mColorDown;
	else {
		float n = mTransitionHeight;
		float l = (majorN + n) / n * 0.5f;
		outClr = mColorDown;
		outClr.Lerp(mColorUp, l);
	}
}

KSC_EnvShader::KSC_EnvShader(const char* shaderTemplate)
{
	mIsValid = LoadTemplate(shaderTemplate);
}

KSC_EnvShader::~KSC_EnvShader()
{

}

void KSC_EnvShader::Sample(const KVec3& pos, const KVec3& dir, KColor& outClr) const
{
	Execute(mEnvContext.mpData, &outClr);
}

bool KSC_EnvShader::IsValid() const
{
	return mIsValid;
}

bool KSC_EnvShader::Validate(FunctionHandle shadeFunc)
{
	// Perform the extra check for the second and third arguments
	if (3 != KSC_GetFunctionArgumentCount(shadeFunc))
		return false;

	KSC_TypeInfo argType0 = KSC_GetFunctionArgumentType(shadeFunc, 1);
	if (!argType0.isRef || !argType0.isKSCLayout || argType0.hStruct == NULL || 
		0 != strcmp(argType0.typeString, "EnvContext") ) {
		printf("Incorrect type for second argument, it must be EnvContext&.\n");
		return false;
	}

	KSC_TypeInfo argType1 = KSC_GetFunctionArgumentType(shadeFunc, 2);
	if (!argType1.isRef || argType1.isKSCLayout || argType1.type != SC::kFloat3) {
		printf("Incorrect type for second argument, it must be float3&.\n");
		return false;
	}

	return true;
}

bool KSC_EnvShader::HandleModule(ModuleHandle kscModule)
{
	FunctionHandle shadeFunc = KSC_GetFunctionHandleByName("Shade", kscModule);
	KSC_TypeInfo argType1 = KSC_GetFunctionArgumentType(shadeFunc, 1);
	mEnvContext.Allocate(argType1);
	return true;
}
