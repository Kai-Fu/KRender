#pragma once
#include "../shader/surface_shader.h"
#include "../image/basic_map.h"

#define BASIC_PHONG "basic_phong"
#define MIRROR		"mirror"
#define DIAGNOSTIC	"diagnostic"
#define SHADER_SURFACE "shader_surface"

class KRT_API PhongSurface : public ISurfaceShader
{
public:

	struct PARAM {
		KColor mDiffuse;
		KColor mSpecular;
		float mPower;
		KColor mOpacity;
	};
	PARAM mParam;

	std::string mDiffuseMapFileName;
	Texture::Tex2D* mDiffuseMap;

	std::string mNormalMapFileName;

	PhongSurface(const char* name);
	virtual ~PhongSurface() {}

	virtual void CalculateShading(const KColor& in_clr, const KVec3& in_dir, const ShadingContext& shadingCtx, KColor& out_clr) const;
	virtual void SetParam(const char* paramName, void* pData, UINT32 dataSize);

	virtual bool Save(FILE* pFile);
	virtual bool Load(FILE* pFile);

};

class KRT_API MirrorSurface : public ISurfaceShader
{
public:
	MirrorSurface(const char* name);
	virtual ~MirrorSurface();

	struct PARAM {
		KVec3 mBaseColor;
		float mReflectGain;
	};
	PARAM mParam;

	virtual void CalculateShading(const KColor& in_clr, const KVec3& in_dir, const ShadingContext& shadingCtx, KColor& out_clr) const;
	virtual void SetParam(const char* paramName, void* pData, UINT32 dataSize);

	virtual bool Save(FILE* pFile);
	virtual bool Load(FILE* pFile);
};

class KRT_API AttributeDiagnoseSurface : public ISurfaceShader
{
public:
	AttributeDiagnoseSurface(const char* name);
	virtual ~AttributeDiagnoseSurface();
	
	enum Mode {eShowNormal, eShowUV};
	Mode mMode;

	virtual void CalculateShading(const KColor& in_clr, const KVec3& in_dir, const ShadingContext& shadingCtx, KColor& out_clr) const;
	virtual void SetParam(const char* paramName, void* pData, UINT32 dataSize);

	virtual bool Save(FILE* pFile);
	virtual bool Load(FILE* pFile);
};

