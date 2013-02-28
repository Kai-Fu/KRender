#pragma once
#include "../shader/surface_shader.h"
#include "../util/unique_string.h"
#include "../shader/surface_shader_api.h"
#include <common/defines/stl_inc.h>
#include <string>



class KRT_API Material_Library
{
public:
	Material_Library();
	~Material_Library();

	ISurfaceShader* CreateMaterial(const char* pMtlType, const char* pMtlName);
	ISurfaceShader* OpenMaterial(const char* pMtlName);
	
	static Material_Library* GetInstance();
	static void Initialize();
	static void Shutdown();

	bool Save(FILE* pFile);
	bool Load(FILE* pFile);

	void Clear();

private:
	typedef STDEXT::hash_map<std::string, ISurfaceShader*> MTL_MAP;
	MTL_MAP mMaterialInstances;
	UniqueStringMaker mUniqueStrMaker;

	static Material_Library* s_pInstance;
};


