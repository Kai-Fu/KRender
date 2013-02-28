#include "standard_mtl.h"
#include "../shader/light_scheme.h"
#include "../file_io/file_io_template.h"


PhongSurface::PhongSurface(const char* name) :
	ISurfaceShader(BASIC_PHONG, name)
{
	mParam.mDiffuse = KColor(0.5f, 0.5f, 0.5f);
	mParam.mSpecular = KColor(0.7f, 0.7f, 0.7f);
	mParam.mPower = 30.0f;
	mParam.mOpacity = KColor(1, 1, 1);;
	mDiffuseMap = NULL;
}

void PhongSurface::SetParam(const char* paramName, void* pData, UINT32 dataSize)
{
	if (0 == strcmp(paramName, "diffuse_color")) {
		if (dataSize == sizeof(KColor))
			mParam.mDiffuse = *(KColor*)pData;
	}
	else if (0 == strcmp(paramName, "specular_color")) {
		if (dataSize == sizeof(KColor))
			mParam.mSpecular = *(KColor*)pData;
	}
	else if (0 == strcmp(paramName, "power")) {
		if (dataSize == sizeof(float))
			mParam.mPower = *(float*)pData;
	}
	else if (0 == strcmp(paramName, "opacity")) {
		if (dataSize == sizeof(KColor)) {
			mParam.mOpacity = *(KColor*)pData;
		}
	}
	else if (0 == strcmp(paramName, "diffuse_map")) {
		mDiffuseMapFileName = (const char*)pData;
		if (!mDiffuseMapFileName.empty())
			mDiffuseMap = Texture::TextureManager::GetInstance()->CreateBitmapTexture(mDiffuseMapFileName.c_str());
	}

}

bool PhongSurface::Save(FILE* pFile)
{
	std::string typeName = "PhongSurface";
	if (!SaveArrayToFile(typeName, pFile))
		return false;
	
	if (!SaveTypeToFile(mParam, pFile))
		return false;

	if (!SaveArrayToFile(mDiffuseMapFileName, pFile))
		return false;

	if (!SaveArrayToFile(mNormalMapFileName, pFile))
		return false;

	return true;
}

bool PhongSurface::Load(FILE* pFile)
{
	std::string srcTypeName;
	std::string dstTypeName = "PhongSurface"; 
	if (!LoadArrayFromFile(srcTypeName, pFile))
		return false;
	if (srcTypeName.compare(dstTypeName) != 0)
		return false;

	if (!LoadTypeFromFile(mParam, pFile))
		return false;

	if (!LoadArrayFromFile(mDiffuseMapFileName, pFile))
		return false;

	if (!LoadArrayFromFile(mNormalMapFileName, pFile))
		return false;
	
	if (!mDiffuseMapFileName.empty())
		mDiffuseMap = Texture::TextureManager::GetInstance()->CreateBitmapTexture(mDiffuseMapFileName.c_str());

	if (!mNormalMapFileName.empty())
		mNormalMap = Texture::TextureManager::GetInstance()->CreateBitmapTexture(mNormalMapFileName.c_str());

	return true;
}

MirrorSurface::MirrorSurface(const char* name) : ISurfaceShader(MIRROR, name)
{
	mParam.mBaseColor = KVec3(0,0,0);
	mParam.mReflectGain = 1.0f;
}

MirrorSurface::~MirrorSurface()
{

}

void MirrorSurface::CalculateShading(const KColor& in_clr, const KVec3& in_dir, const ShadingContext& shadingCtx, KColor& out_clr) const
{
	KColor reflectColor;
	//CalcReflectedRay(pLocalData, shadingCtx, reflectColor);
	//in_out_clr = reflectColor;
}

void MirrorSurface::SetParam(const char* paramName, void* pData, UINT32 dataSize)
{
	if (0 == strcmp(paramName, "base_color")) {
		if (dataSize == sizeof(KVec3))
			mParam.mBaseColor = *(KVec3*)pData;
	}
	else if (0 == strcmp(paramName, "reflection_gain")) {
		if (dataSize == sizeof(float))
			mParam.mReflectGain = *(float*)pData;
	}
}

bool MirrorSurface::Save(FILE* pFile)
{
	std::string typeName = "MirrorSurface";
	if (!SaveArrayToFile(typeName, pFile))
		return false;
	
	if (!SaveTypeToFile(mParam, pFile))
		return false;

	return true;
}

bool MirrorSurface::Load(FILE* pFile)
{
	std::string srcTypeName;
	std::string dstTypeName = "MirrorSurface"; 
	if (!LoadArrayFromFile(srcTypeName, pFile))
		return false;
	if (srcTypeName.compare(dstTypeName) != 0)
		return false;

	if (!LoadTypeFromFile(mParam, pFile))
		return false;

	return true;
}

AttributeDiagnoseSurface::AttributeDiagnoseSurface(const char* name) : ISurfaceShader(DIAGNOSTIC, name)
{
	mMode = eShowUV;
}

AttributeDiagnoseSurface::~AttributeDiagnoseSurface()
{
}

void AttributeDiagnoseSurface::CalculateShading(const KColor& in_clr, const KVec3& in_dir, const ShadingContext& shadingCtx, KColor& out_clr) const
{
	KVec3 temp;
	switch (mMode) {
	case eShowNormal:
		temp = shadingCtx.normal * 0.5f + KVec3(0.5f, 0.5f, 0.5f);
		out_clr.r = temp[0];
		out_clr.g = temp[1];
		out_clr.b = temp[2];
		break;
	case eShowUV:
		if (shadingCtx.hasUV) {
			temp = shadingCtx.tangent.tangent * 0.5f + KVec3(0.5f, 0.5f, 0.5f);
			out_clr.r = temp[0];
			out_clr.g = temp[1];
			out_clr.b = temp[2];
		}
		else {
			out_clr.r = 1;
			out_clr.g = 1;
			out_clr.b = 1;
		}
		break;
	default:
		out_clr.r = 1;
		out_clr.g = 1;
		out_clr.b = 1;
		break;
	}
	
}

void AttributeDiagnoseSurface::SetParam(const char* paramName, void* pData, UINT32 dataSize)
{

}

bool AttributeDiagnoseSurface::Save(FILE* pFile)
{
	std::string typeName = "AttributeDiagnoseSurface";
	if (!SaveArrayToFile(typeName, pFile))
		return false;

	UINT32 mode = (UINT32)mMode;
	if (!SaveTypeToFile(mode, pFile))
		return false;

	return true;
}

bool AttributeDiagnoseSurface::Load(FILE* pFile)
{
	std::string srcTypeName;
	std::string dstTypeName = "AttributeDiagnoseSurface"; 
	if (!LoadArrayFromFile(srcTypeName, pFile))
		return false;
	if (srcTypeName.compare(dstTypeName) != 0)
		return false;

	UINT32 mode;
	if (!LoadTypeFromFile(mode, pFile))
		return false;
	mMode = (Mode)mode;
	return true;
}

