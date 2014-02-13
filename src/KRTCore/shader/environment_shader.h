#pragma once

#include "shader_api.h"
#include "../image/basic_map.h"

class KEnvShader
{
public:
	KEnvShader();
	virtual ~KEnvShader();

	virtual void Sample(const EnvContext& ctx, KColor& outClr) const = 0;

	static const KEnvShader* GetEnvShader();
	static bool Initialize();
	static void Shutdown();

	static bool SetEnvironmentShader(const char* templateFile);

private:
	static KEnvShader* s_currentEvnShader;
};


class KSC_EnvShader : public KEnvShader, public KSC_ShaderWithTexture
{
public:
	KSC_EnvShader(const char* shaderTemplate);
	virtual ~KSC_EnvShader();
	virtual void Sample(const EnvContext& ctx, KColor& outClr) const;

	bool IsValid() const;

	// From KSC_ShaderWithTexture
	virtual bool Validate(FunctionHandle shadeFunc);
	virtual bool HandleModule(ModuleHandle kscModule);

private:
	bool mIsValid;
};