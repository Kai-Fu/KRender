#include "material_library.h"
#include "standard_mtl.h"
#include "../file_io/file_io_template.h"
#include "../shader/light_scheme.h"

Material_Library* Material_Library::s_pInstance = NULL;

Material_Library::Material_Library()
{
}

Material_Library::~Material_Library()
{

}

ISurfaceShader* Material_Library::CreateMaterial(const char* pMtlType, const char* pMtlName)
{
	if (pMtlType && pMtlName) {
		std::string mtlName;
		mUniqueStrMaker.MakeUniqueString(mtlName, pMtlName);
		ISurfaceShader* pRet = NULL;
		if (0 == strcmp(pMtlType, BASIC_PHONG)) {
			pRet = new PhongSurface(mtlName.c_str());
			mMaterialInstances[mtlName] = pRet;
		}
		else if (0 == strcmp(pMtlType, MIRROR)) {
			pRet = new MirrorSurface(mtlName.c_str());
			mMaterialInstances[mtlName] = pRet;
		}
		else if (0 == strcmp(pMtlType, DIAGNOSTIC)) {
			pRet = new AttributeDiagnoseSurface(mtlName.c_str());
			mMaterialInstances[mtlName] = pRet;
		}
			
		return pRet;
	}
	else
		return NULL;
}

ISurfaceShader* Material_Library::OpenMaterial(const char* pMtlName)
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

void Material_Library::Clear()
{
	MTL_MAP::iterator it = mMaterialInstances.begin();
	for (; it != mMaterialInstances.end(); ++it) {
		delete it->second;
	}
	mMaterialInstances.clear();
	mUniqueStrMaker.Clear();
}

bool Material_Library::Save(FILE* pFile)
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

bool Material_Library::Load(FILE* pFile)
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

Material_Library* Material_Library::GetInstance()
{
	return s_pInstance;
}

void Material_Library::Initialize()
{
	s_pInstance = new Material_Library();
}

void Material_Library::Shutdown()
{
	delete s_pInstance;
	s_pInstance = NULL;
}
