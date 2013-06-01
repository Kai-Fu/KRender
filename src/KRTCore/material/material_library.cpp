#include "material_library.h"
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

		KSC_SurfaceShader* pRet = NULL;
		if (mShaderDefaultInstances.find(shaderTemplate) == mShaderDefaultInstances.end()) {
			KSC_SurfaceShader* pShader = new KSC_SurfaceShader(shaderTemplate, mtlName.c_str());
			if (pShader->LoadAndCompile()) {
				mShaderDefaultInstances[shaderTemplate] = pShader;
				pRet = pShader;
			}
			else
				delete pShader;
		}
		else
			pRet = mShaderDefaultInstances[shaderTemplate];
		
		if (pRet) {
			pRet = new KSC_SurfaceShader(*pRet);
			pRet->SetName(pMtlName);
			mMaterialInstances[mtlName] = pRet;
		}

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
	mpEmissionFuncPtr = NULL;
	mpTransmissionFuncPtr = NULL;
}

KSC_SurfaceShader::KSC_SurfaceShader(const KSC_SurfaceShader& ref) :
	ISurfaceShader(ref.mTypeName.c_str(), ref.mName.c_str()), KSC_ShaderWithTexture(ref)
{
	mpEmissionFuncPtr = ref.mpEmissionFuncPtr;
	mpTransmissionFuncPtr = ref.mpTransmissionFuncPtr;
	mNormalMap = ref.mNormalMap;
	mRecieveLight = ref.mRecieveLight;
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
	if (!argType0.isRef || !argType0.isKSCLayout || argType0.hStruct == NULL || 
		0 != strcmp(argType0.typeString, "SurfaceContext") ) {
		printf("Incorrect type for second argument, it must be SurfaceContext&.\n");
		return false;
	}

	KSC_TypeInfo argType1 = KSC_GetFunctionArgumentType(shadeFunc, 2);
	if (!argType1.isRef || argType1.isKSCLayout || argType1.type != SC::kFloat3) {
		printf("Incorrect type for second argument, it must be float3&.\n");
		return false;
	}

	return true;
}

bool KSC_SurfaceShader::HandleModule(ModuleHandle kscModule)
{
	// Look for the function "ShadeTransmission(SurfaceContext% ctx, KColor& outClr)"
	FunctionHandle shadeFunc = KSC_GetFunctionHandleByName("ShadeTransmission", kscModule);
	if (shadeFunc != NULL && KSC_GetFunctionArgumentCount(shadeFunc) == 3) {
		KSC_TypeInfo argUniform = KSC_GetFunctionArgumentType(shadeFunc, 0);
		KSC_TypeInfo arg0TypeInfo = KSC_GetFunctionArgumentType(shadeFunc, 1);
		KSC_TypeInfo arg1TypeInfo = KSC_GetFunctionArgumentType(shadeFunc, 2);
		if (0 == strcmp(argUniform.typeString, mUnifomArgType.typeString) &&
			arg0TypeInfo.isRef && arg0TypeInfo.isKSCLayout && arg0TypeInfo.hStruct != NULL &&
			arg1TypeInfo.isRef && arg1TypeInfo.type == SC::kFloat3) {
			mpTransmissionFuncPtr = KSC_GetFunctionPtr(shadeFunc);
			if (mpTransmissionFuncPtr)
				mHasTransmission = true;
		}
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

void KSC_SurfaceShader::Shade(const SurfaceContext& shadingCtx, KColor& out_clr) const
{
	Execute(shadingCtx.mpData, &out_clr);
}

void KSC_SurfaceShader::ShaderTransmission(const TransContext& shadingCtx, KColor& out_clr) const
{
	typedef void (*PFN_invoke)(void*, void*, void*);
	PFN_invoke funcPtr = (PFN_invoke)mpTransmissionFuncPtr;
	funcPtr(mpUniformData, shadingCtx.mpData, &out_clr);
}

bool KSC_SurfaceShader::Save(FILE* pFile)
{
	UINT64 paramCnt = mModifiedData.size();
	if (!SaveTypeToFile(paramCnt, pFile))
		return false;

	std::hash_map<std::string, std::vector<BYTE> >::const_iterator it = mModifiedData.begin();
	for (; it != mModifiedData.end(); ++it) {
		if (!SaveArrayToFile(it->first, pFile))
			return false;
		if (!SaveArrayToFile(it->second, pFile))
			return false;
	}
	return true;
}

bool KSC_SurfaceShader::Load(FILE* pFile)
{
	UINT64 paramCnt = 0;
	if (!LoadTypeFromFile(paramCnt, pFile))
		return false;

	for (size_t i = 0; i < paramCnt; ++i) {
		std::string paramName;
		if (!LoadArrayFromFile(paramName, pFile))
			return false;
		std::vector<BYTE>& data = mModifiedData[paramName];
		if (!LoadArrayFromFile(data, pFile))
			return false;

		SetUniformParam(paramName.c_str(), &data[0], (int)data.size());
	}
	return true;
}