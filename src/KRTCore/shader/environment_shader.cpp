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
	return SetEnvironmentShader("./asset/environment/atrium.template");
}

void KEnvShader::Shutdown()
{
	if (s_currentEvnShader)
	delete s_currentEvnShader;
	s_currentEvnShader = NULL;
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

KSC_EnvShader::KSC_EnvShader(const char* shaderTemplate)
{
	mIsValid = LoadTemplate(shaderTemplate);
}

KSC_EnvShader::~KSC_EnvShader()
{

}

void KSC_EnvShader::Sample(const EnvContext& ctx, KColor& outClr) const
{
	Execute(ctx.mpData, &outClr);
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
	return true;
}
