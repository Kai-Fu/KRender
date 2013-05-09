#include "material_library.h"
#include "standard_mtl.h"
#include "../file_io/file_io_template.h"
#include "../shader/light_scheme.h"

KMaterialLibrary* KMaterialLibrary::s_pInstance = NULL;

KMaterialLibrary::KMaterialLibrary()
{
}

KMaterialLibrary::~KMaterialLibrary()
{

}

ISurfaceShader* KMaterialLibrary::CreateMaterial(const char* shaderTemplate, const char* pMtlName)
{
	if (shaderTemplate && pMtlName) {
		std::string mtlName;
		mUniqueStrMaker.MakeUniqueString(mtlName, pMtlName);
		KSC_SurfaceShader* pShader = new KSC_SurfaceShader(shaderTemplate, mtlName.c_str());;
		pShader->LoadAndCompile();
		ISurfaceShader* pRet = pShader;

		/*if (0 == strcmp(shaderTemplate, BASIC_PHONG)) {
			pRet = new PhongSurface(mtlName.c_str());
			mMaterialInstances[mtlName] = pRet;
		}
		else if (0 == strcmp(shaderTemplate, MIRROR)) {
			pRet = new MirrorSurface(mtlName.c_str());
			mMaterialInstances[mtlName] = pRet;
		}
		else if (0 == strcmp(shaderTemplate, DIAGNOSTIC)) {
			pRet = new AttributeDiagnoseSurface(mtlName.c_str());
			mMaterialInstances[mtlName] = pRet;
		}*/
		mMaterialInstances[mtlName] = pRet;
		return pRet;
	}
	else
		return NULL;
}

ISurfaceShader* KMaterialLibrary::OpenMaterial(const char* pMtlName)
{
	if (pMtlName) {
		MTL_MAP::iterator it = mMaterialInstances.find(pMtlName);
		if (it != mMaterialInstances.end())
			return it->second;
		else
			return NULL;
	}
	else
		return NULL;
}

void KMaterialLibrary::Clear()
{
	MTL_MAP::iterator it = mMaterialInstances.begin();
	for (; it != mMaterialInstances.end(); ++it) {
		delete it->second;
	}
	mMaterialInstances.clear();
	mUniqueStrMaker.Clear();
}

bool KMaterialLibrary::Save(FILE* pFile)
{
	std::string typeName = "Material_Library";
	if (!SaveArrayToFile(typeName, pFile))
		return false;

	UINT64 cnt = mMaterialInstances.size();
	if (!SaveTypeToFile(cnt, pFile))
		return false;
	MTL_MAP::iterator it = mMaterialInstances.begin();
	for (; it != mMaterialInstances.end(); ++it) {
		std::string typeName = it->second->GetTypeName();
		if (!SaveArrayToFile(typeName, pFile))
			return false;

		if (!SaveArrayToFile(it->first, pFile))
			return false;

		if (!it->second->Save(pFile))
			return false;
	}
	return true;
}

bool KMaterialLibrary::Load(FILE* pFile)
{
	Clear();
	std::string srcTypeName;
	std::string dstTypeName = "Material_Library"; 
	if (!LoadArrayFromFile(srcTypeName, pFile))
		return false;
	if (srcTypeName.compare(dstTypeName) != 0)
		return false;

	UINT64 cnt = 0;
	if (!LoadTypeFromFile(cnt, pFile))
		return false;
	
	for (UINT32 i = 0; i < (UINT32)cnt; ++i) {
		std::string typeName, mtlName;
		if (!LoadArrayFromFile(typeName, pFile))
			return false;

		if (!LoadArrayFromFile(mtlName, pFile))
			return false;

		ISurfaceShader* pMtl = CreateMaterial(typeName.c_str(), mtlName.c_str());
		if (!pMtl)
			return false;

		if (!pMtl->Load(pFile))
			return false;
	}
	return true;
}

KMaterialLibrary* KMaterialLibrary::GetInstance()
{
	return s_pInstance;
}

void KMaterialLibrary::Initialize()
{
	s_pInstance = new KMaterialLibrary();
}

void KMaterialLibrary::Shutdown()
{
	delete s_pInstance;
	s_pInstance = NULL;
}

KSC_SurfaceShader::KSC_SurfaceShader(const char* shaderTemplate, const char* shaderName) :
	ISurfaceShader(shaderTemplate, shaderName)
{

}

KSC_SurfaceShader::~KSC_SurfaceShader()
{

}

bool KSC_SurfaceShader::Validate(FunctionHandle shadeFunc)
{
	// Perform the extra check for the second and third arguments
	if (3 != KSC_GetFunctionArgumentCount(shadeFunc))
		return false;

	KSC_TypeInfo argType0 = KSC_GetFunctionArgumentType(shadeFunc, 1);
	if (!argType0.isRef || !argType0.isKSCLayout) {
		printf("Incorrect type for second argument, it must be SurfaceContext&.\n");
		return false;
	}

	KSC_TypeInfo argType1 = KSC_GetFunctionArgumentType(shadeFunc, 2);
	if (!argType1.isRef || argType1.type != SC::kFloat3) {
		printf("Incorrect type for second argument, it must be float3&.\n");
		return false;
	}

	return true;
}

bool KSC_SurfaceShader::LoadAndCompile()
{
	return LoadTemplate(mTypeName.c_str());
}

void KSC_SurfaceShader::SetParam(const char* paramName, void* pData, UINT32 dataSize)
{
	SetUniformParam(paramName, pData, dataSize);
}

void KSC_SurfaceShader::CalculateShading(const SurfaceContext& shadingCtx, KColor& out_clr) const
{
	Execute(shadingCtx.mpData, &out_clr);
}

bool KSC_SurfaceShader::Save(FILE* pFile)
{
	return true;
}

bool KSC_SurfaceShader::Load(FILE* pFile)
{
	return true;
}