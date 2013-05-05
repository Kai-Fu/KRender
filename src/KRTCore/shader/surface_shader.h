#pragma once

#include "../base/geometry.h"
#include "shader_api.h"

#include "../image/basic_map.h"
#include <string>


// The main entry function to calculate the shading for the specified ray
bool CalcuShadingByRay(TracingInstance* pLocalData, const KRay& ray, KColor& out_clr, IntersectContext* out_ctx = NULL);
bool CalcReflectedRay(TracingInstance* pLocalData, const ShadingContext& shadingCtx, KColor& reflectColor);
bool CalcRefractedRay(TracingInstance* pLocalData, const ShadingContext& shadingCtx, float refractRatio, KColor& refractColor);


class ISurfaceShader
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
