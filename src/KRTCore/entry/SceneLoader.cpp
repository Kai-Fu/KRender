#include "SceneLoader.h"

#include "../file_io/KObjFileLoader.h"
#include "../file_io/file_io_template.h"
#include "../material/material_library.h"
#include "../camera/camera_manager.h"
#include "../shader/light_scheme.h"
#include "../util/HelperFunc.h"

#include <assert.h>

#include "../image/basic_map.h"

namespace KRayTracer {


SceneLoader::SceneLoader()
{
	LightScheme::Initialize();
	Material_Library::Initialize();
	CameraManager::Initialize();
	Texture::TextureManager::Initialize();

	mpScene = new KKDBBoxScene();
}

SceneLoader::~SceneLoader()
{
	LightScheme::Shutdown();
	Material_Library::Shutdown();
	CameraManager::Shutdown();
	Texture::TextureManager::Shutdown();

	if (mpScene)
		delete mpScene;
}


class KRT_ObjFileLoader : public KObjFileLoader
{
protected:
	virtual void ProcessingMaterial(GLMmodel* model, KScene& scene, const stdext::hash_map<UINT32, UINT32>& nodeToMtl)
	{
		// Create material
		Material_Library* pML = Material_Library::GetInstance();
		for (UINT32 mi = 0; mi < model->nummaterials; ++mi) {
			ISurfaceShader* pSurfShader = pML->CreateMaterial("basic_phong", model->materials[mi].name);
			pSurfShader->SetParam("diffuse_color", model->materials[mi].diffuse, sizeof(float)*3);
			if (model->materials[mi].shininess > 3.0f) {
				pSurfShader->SetParam("specular_color", model->materials[mi].specular, sizeof(float)*3);
				pSurfShader->SetParam("power", &model->materials[mi].shininess, sizeof(float));
			}
			else {
				float specular[3] = {0,0,0};
				float pow = 0;
				pSurfShader->SetParam("specular_color", specular, sizeof(float)*3);
				pSurfShader->SetParam("power", &pow, sizeof(float));
			}

			pSurfShader->SetParam("opacity", model->materials[mi].transparency, sizeof(float)*3);

			if (mUseTexMap && model->materials[mi].diffuse_map) {
				pSurfShader->SetParam("diffuse_map", model->materials[mi].diffuse_map, 0);
			}
		}

		for (UINT32 i = 0; i < scene.GetNodeCnt(); ++i) {
			KNode* pNode = scene.GetNode(i);
			stdext::hash_map<UINT32, UINT32>::const_iterator it = nodeToMtl.find(i);
			assert(it != nodeToMtl.end());
			pNode->mpSurfShader = pML->OpenMaterial(model->materials[it->second].name);
		}

	}
};

#define FILE_EXT_UNKNOWN -1
#define FILE_EXT_SCN	1
#define FILE_EXT_OBJ	2
static int _FileExtension(const char* file_name)
{
	std::string pathname(file_name);
	size_t dot_pos = pathname.rfind('.');
	if (dot_pos != std::string::npos) {
		const char* ext = &pathname.c_str()[dot_pos + 1];
		if (strcmp(ext, "obj") == 0)
			return FILE_EXT_OBJ;
		else if (strcmp(ext, "scn") == 0)
			return FILE_EXT_SCN;
		else
			return FILE_EXT_UNKNOWN;
	}
	else
		return FILE_EXT_UNKNOWN;
}

bool SceneLoader::LoadFromFile(const char* file_name)
{
	KTimer stop_watch(true);
	FILE* pFile = NULL;
	bool ret = false;
	int ext = _FileExtension(file_name);

	std::string file_dir;
	GetPathDir(file_name, file_dir);
	Texture::TextureManager::GetInstance()->AddSearchPath(file_dir.c_str());

	if (!mpScene)
		mpScene = new KKDBBoxScene;
	else
		mpScene->Reset();

	KTimer fileReadingTime(true);
	// Perform the file reading
	if (ext == FILE_EXT_SCN) {
		fopen_s(&pFile, file_name, "rb");
		if (pFile)
			ret = LoadAsSCN(pFile);
		else
			ret = false;
	}
	else if (ext == FILE_EXT_OBJ) {
		KRT_ObjFileLoader OBJLoader;
		OBJLoader.mUseTexMap = USE_TEX_MAP ? true : false;

		// Create scene
		UINT32 kd_idx = 0;
		KKDTreeScene* pKDScene = mpScene->AddKDScene(kd_idx);
		mpScene->SceneNode_Create(kd_idx);

		if (OBJLoader.LoadObjFile(file_name, *pKDScene)) {
			ret = true;
		}
		else {
			mpScene->Reset();
			ret = false;
		}
	}
	mFileLoadingTime = UINT32(fileReadingTime.Stop() * 1000);

	// End of file reading, now build the acceleration structure
	mpScene->SceneNode_BuildAccelData(true);

	KBBox scene_box = mpScene->GetSceneBBox();
	KVec3 center = scene_box.Center();
	float radius = nvmath::length(scene_box.mMax - scene_box.mMin) * 1.0f;
	
	CameraManager* pCameraMan = CameraManager::GetInstance();
	if (pCameraMan->GetCameraCnt() == 0 && mpScene) {
		// If there's no camera, create a default light regarding the bounding box of scene
		KCamera* pPinHoleCamera = pCameraMan->OpenCamera("__default", true);
		KCamera::MotionState ms;
		ms.pos = center + KVec3(0.5,0.5,0.5)*radius;
		ms.lookat = center;
		ms.up = KVec3(0, 1.0f, 0);
		ms.xfov = 45.0f;
		ms.focal = radius * 0.5f;
		pPinHoleCamera->SetupStillCamera(ms);
	}
	
	LightScheme* pLightScheme = LightScheme::GetInstance();
	if (pLightScheme->GetLightCount() == 0 && mpScene) {
		// If there's no light source, create a default one, otherwize the scene will be entirely dark.
		PointLightBase* pLight0 = dynamic_cast<PointLightBase*>(pLightScheme->CreateLightSource(POINT_LIGHT_TYPE));
		PointLightBase* pLight1 = dynamic_cast<PointLightBase*>(pLightScheme->CreateLightSource(POINT_LIGHT_TYPE));
		pLight0->SetIntensity(KColor(0.55f, 0.55f, 0.55f));
		pLight1->SetIntensity(KColor(0.55f, 0.55f, 0.55f));
		pLight0->SetPos(center + KVec3(0,1,1)*(radius*3.0f));
		pLight1->SetPos(center + KVec3(1,1,0)*(radius*3.0f));
	}

	if (pFile)
		fclose(pFile);

	mLoadingTime = stop_watch.Stop();

	return ret;
}

bool SceneLoader::SaveAsSCN(FILE* pFile) const
{
	if (!pFile)
		return false;

	if (!Material_Library::GetInstance()->Save(pFile))
		return false;

	if (!CameraManager::GetInstance()->Save(pFile))
		return false;

	if (!LightScheme::GetInstance()->Save(pFile))
		return false;
		
	// Then save the scene
	if (mpScene)
		return mpScene->SaveToFile(pFile);
	else
		return false;
}

bool SceneLoader::LoadAsSCN(FILE* pFile)
{
	if (!pFile)
		return false;

	if (!Material_Library::GetInstance()->Load(pFile))
		return false;

	if (!CameraManager::GetInstance()->Load(pFile))
		return false;

	if (!LightScheme::GetInstance()->Load(pFile))
		return false;
		
	// Then load the scene
	bool ret = mpScene->LoadFromFile(pFile);
	return ret;
}

bool SceneLoader::SaveToFile(const char* file_name) const
{
	FILE* pFile = NULL;
	// Save the update of rigid body animation nodes
	fopen_s(&pFile, file_name, "wb");
	if (!pFile) return false;

	bool res = SaveAsSCN(pFile);
	fclose(pFile);
	return res;
}

bool SceneLoader::SaveUpdatingFile(const char* file_name, 
			const std::vector<UINT32>& modified_cameras,
			const std::vector<UINT32>& modified_lights,
			const std::vector<UINT32>& modified_morph_nodes, 
			const std::vector<UINT32>& modified_RBA_nodes) const
{
	FILE* pFile = NULL;
	// Save the update of rigid body animation nodes
	fopen_s(&pFile, file_name, "wb");
	if (!pFile) return false;

	std::string tag("KRT_Updating");
	if (!SaveArrayToFile(tag, pFile))
		return false;
	// TODO: handling of camera and light updating

	if (!SaveArrayToFile(modified_RBA_nodes, pFile))
		return false;
	
	if (!mpScene->SceneNode_SaveUpdates(modified_RBA_nodes, modified_morph_nodes, pFile))
		return false;

	fclose(pFile);
	return true;
}

bool SceneLoader::LoadUpdatingFile(const char* file_name)
{
	FILE* pFile = NULL;
	fopen_s(&pFile, file_name, "rb");
	if (!pFile)
		return false;

	std::string srcTypeName;
	std::string dstTypeName = "KRT_Updating"; 
	if (!LoadArrayFromFile(srcTypeName, pFile))
		return false;
	if (srcTypeName.compare(dstTypeName) != 0)
		return false;
	// TODO: handling of camera and light updating

	std::vector<UINT32> modified_RBA_nodes;
	if (!LoadArrayFromFile(modified_RBA_nodes, pFile))
		return false;
	
	if (!mpScene)
		return false; // Need to load a scene before loading update file

	if (!mpScene->SceneNode_LoadUpdates(pFile))
		return false;

	fclose(pFile);
	return true;
}



}