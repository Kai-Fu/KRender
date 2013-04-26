#pragma once

#include "../base/geometry.h"
#include "surface_shader_api.h"

#include "../image/basic_map.h"
#include <string>

#define SHADING_FLAG_DIFFUSE_ONLY	0x0001
#define SHADING_FLAG_NO_TEX_FILTER	0x0002
#define SHADING_IC_SAMP_RAY					0x0004
#define SHADING_IC_GATHER						0x0008



// The main entry function to calculate the shading for the specified ray
bool KRT_API CalcuShadingByRay(TracingInstance* pLocalData, const KRay& ray, KColor& out_clr, IntersectContext* out_ctx = NULL);
bool KRT_API CalcReflectedRay(TracingInstance* pLocalData, const ShadingContext& shadingCtx, KColor& reflectColor);
bool KRT_API CalcRefractedRay(TracingInstance* pLocalData, const ShadingContext& shadingCtx, float refractRatio, KColor& refractColor);


class KRT_API ISurfaceShader
{
protected:
	std::string mTypeName;
	std::string mName;

public:
	// The surface shader implementation need to set the normal map
	Texture::Tex2D* mNormalMap;

public:
	ISurfaceShader(const char* typeName, const char* name) : 
		mTypeName(typeName), 
		mName(name),
		mNormalMap(NULL)
		{}
	virtual ~ISurfaceShader() {}

	virtual void SetParam(const char* paramName, void* pData, UINT32 dataSize) {}

	virtual void CalculateShading(const KColor& in_clr, const KVec3& in_dir, const ShadingContext& shadingCtx, KColor& out_clr) const = 0;

	virtual bool Save(FILE* pFile) = 0;
	virtual bool Load(FILE* pFile) = 0;

	const char* GetTypeName() const {return mTypeName.c_str();}
	const char* GetName() const {return mName.c_str();}

};
