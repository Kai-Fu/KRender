#include "material_library.h"
#include "../file_io/file_io_template.h"
#include "../shader/light_scheme.h"

KMaterialLibrary* KMaterialLibrary::s_pInstance = NULL;

KMaterialLibrary::KMaterialLibrary()
{
	mpDefaultShader = NULL;
}

KMaterialLibrary::~KMaterialLibrary()
{

}

ISurfaceShader* KMaterialLibrary::CreateMaterial(const char* templateName, const char* pMtlName)
{
	if (templateName && pMtlName) {
		std::string mtlName;
		mUniqueStrMaker.MakeUniqueString(mtlName, pMtlName);

		KSC_SurfaceShader* pRet = NULL;
		if (mShaderTemplates.find(templateName) == mShaderTemplates.end()) {
			KSC_SurfaceShader* pShader = new KSC_SurfaceShader(templateName, mtlName.c_str());
			if (pShader->LoadAndCompile()) {
				mShaderTemplates[templateName] = pShader;
				pRet = pShader;
			}
			else
				delete pShader;
		}
		else
			pRet = mShaderTemplates[templateName];
		
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

ISurfaceShader* KMaterialLibrary::GetDefaultMaterial()
{
	return mpDefaultShader;
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

KMaterialLibrary* KMaterialLibrary::GetInstance()
{
	return s_pInstance;
}

bool KMaterialLibrary::Initialize()
{
	s_pInstance = new KMaterialLibrary();
	s_pInstance->mpDefaultShader = s_pInstance->CreateMaterial("simple_phong_default.template", "");
	if (s_pInstance->mpDefaultShader)
		return true;
	else 
		return false;
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
	mHasTransmission = ref.mHasTransmission;
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
	if (mHasTransmission) {
		typedef void (*PFN_invoke)(void*, void*, void*);
		PFN_invoke funcPtr = (PFN_invoke)mpTransmissionFuncPtr;
		funcPtr(mpUniformData, shadingCtx.mpData, &out_clr);
	}
	else
		out_clr.Clear();
}
