#pragma once
#include "../shader/surface_shader.h"
#include "../util/unique_string.h"
#include "../shader/shader_api.h"
#include <common/defines/stl_inc.h>
#include <string>


class KMaterialLibrary
{
public:
	KMaterialLibrary();
	~KMaterialLibrary();

	ISurfaceShader* CreateMaterial(const char* shaderTemplate, const char* pMtlName);
	ISurfaceShader* OpenMaterial(const char* pMtlName);
	
	static KMaterialLibrary* GetInstance();
	static void Initialize();
	static void Shutdown();

	bool Save(FILE* pFile);
	bool Load(FILE* pFile);

	void Clear();

private:
	typedef std_hash_map<std::string, ISurfaceShader*> MTL_MAP;
	MTL_MAP mMaterialInstances;
	UniqueStringMaker mUniqueStrMaker;

	static KMaterialLibrary* s_pInstance;
};

class KSC_SurfaceShader : public ISurfaceShader, public KSC_ShaderWithTexture
{
public:
	KSC_SurfaceShader(const char* shaderTemplate, const char* shaderName);
	~KSC_SurfaceShader();

	bool LoadAndCompile();

	// From KSC_ShaderWithTexture
	virtual bool Validate(FunctionHandle shadeFunc);

	// From ISurfaceShader
	virtual void SetParam(const char* paramName, void* pData, UINT32 dataSize);
	virtual void CalculateShading(const SurfaceContext& shadingCtx, KColor& out_clr) const;
};
