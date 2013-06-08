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


	static void UseSpheeEnvironment(const KColor& upClr, const KColor& downClr, float transHeight);
	static void UseCubemapEnvironment(const char* filename);

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

class KCubeEvnShader : public KEnvShader
{
public:
	KCubeEvnShader(const char* filename);
	virtual ~KCubeEvnShader();

	virtual void Sample(const KVec3& pos, const KVec3& dir, KColor& outClr) const;

private:
	Texture::TexCube mCubeMap;
};