#pragma once

#include "shader_api.h"
#include "../image/basic_map.h"

class KEnvShader
{
public:
	KEnvShader();
	virtual ~KEnvShader();

	virtual void Sample(const KVec3& pos, const KVec3& dir, KColor& outClr) const = 0;

	static const KEnvShader* GetEnvShader();
	static bool Initialize();
	static void Shutdown();


	static void UseSphereEnvironment(const KColor& upClr, const KColor& downClr, float transHeight);

	static bool SetEnvironmentShader(const char* templateFile);

private:
	static KEnvShader* s_currentEvnShader;
};

class KHemishpereEnvShader : public KEnvShader
{
public:
	KHemishpereEnvShader(const KColor& upClr, const KColor& downClr, float transHeight);
	virtual ~KHemishpereEnvShader();

	virtual void Sample(const KVec3& pos, const KVec3& dir, KColor& outClr) const;

private:
	KColor mColorUp;
	KColor mColorDown;
	float mTransitionHeight;
};

class KSC_EnvShader : public KEnvShader, public KSC_ShaderWithTexture
{
public:
	KSC_EnvShader(const char* shaderTemplate);
	virtual ~KSC_EnvShader();
	virtual void Sample(const KVec3& pos, const KVec3& dir, KColor& outClr) const;

	bool IsValid() const;

	// From KSC_ShaderWithTexture
	virtual bool Validate(FunctionHandle shadeFunc);
	virtual bool HandleModule(ModuleHandle kscModule);

private:
	bool mIsValid;
	EnvContext mEnvContext;
};