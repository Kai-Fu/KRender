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
	KMaterialLibrary::Initialize();
	CameraManager::Initialize();
	Texture::TextureManager::Initialize();

	mIsFromOBJ = false;
	mIsSceneLoaded = false;
	mpScene = new KSceneSet();
	mpAccelData = NULL;
}

SceneLoader::~SceneLoader()
{
	LightScheme::Shutdown();
	KMaterialLibrary::Shutdown();
	CameraManager::Shutdown();
	Texture::TextureManager::Shutdown();

	if (mpScene)
		delete mpScene;

	if (mpAccelData)
		delete mpAccelData;
}


class KRT_ObjFileLoader : public KObjFileLoader
{
protected:
	virtual void ProcessingMaterial(GLMmodel* model, KScene& scene, const stdext::hash_map<UINT32, UINT32>& nodeToMtl)
	{
		// Create material
		KMaterialLibrary* pML = KMaterialLibrary::GetInstance();
		for (UINT32 mi = 0; mi < model->nummaterials; ++mi) {
			ISurfaceShader* pSurfShader = pML->CreateMaterial("simple_phong_default.template", model->materials[mi].name);
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
#define FILE_EXT_OBJ	1
#define FILE_EXT_ABC	2
static int _FileExtension(const char* file_name)
{
	std::string pathname(file_name);
	size_t dot_pos = pathname.rfind('.');
	if (dot_pos != std::string::npos) {
		const char* ext = &pathname.c_str()[dot_pos + 1];
		if (strcmp(ext, "obj") == 0)
			return FILE_EXT_OBJ;
		else if (strcmp(ext, "abc") == 0)
			return FILE_EXT_ABC;
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
		mpScene = new KSceneSet;
	else
		mpScene->Reset();

	mIsFromOBJ = false;
	mIsSceneLoaded = false;
	KTimer fileReadingTime(true);
	// Perform the file reading
	if (ext == FILE_EXT_OBJ) {
		KRT_ObjFileLoader OBJLoader;
		OBJLoader.mUseTexMap = USE_TEX_MAP ? true : false;

		// Create scene
		UINT32 kd_idx = 0;
		KScene* pKDScene = mpScene->AddKDScene(kd_idx);
		mpScene->SceneNode_Create(kd_idx);

		if (OBJLoader.LoadObjFile(file_name, *pKDScene)) {
			ret = true;
			mIsFromOBJ = true;
		}
		else {
			mpScene->Reset();
			ret = false;
		}
	}
	else if (ext == FILE_EXT_ABC) {
		if (mAbcLoader.Load(file_name, *mpScene))
			ret = true;
		else {
			mpScene->Reset();
			ret = false;
		}
	}

	if (ret) {
		BuildNodeIdMap();
		mIsSceneLoaded = true;
	}

	mFileLoadingTime = UINT32(fileReadingTime.Stop() * 1000);

	// End of file reading, now build the acceleration structure
	mpAccelData = new KAccelStruct_BVH(mpScene);
	mpAccelData->SceneNode_BuildAccelData(NULL);

	KBBox scene_box = mpAccelData->GetSceneBBox();
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

bool SceneLoader::UpdateTime(double timeInSec, double duration)
{
	if (!mIsSceneLoaded) {
		std::cout << "Scene not loaded...it cannot be updated." << std::endl;
		return false;
	}

	if (mIsFromOBJ) {
		std::cout << "Scene loaded from OBJ file cannot be updated." << std::endl;
		return false;
	}
		
	std::list<UINT32> changedScenes;
	if (mAbcLoader.Update((float)timeInSec, (float)duration, changedScenes)) {
		// Let's update the accellerating structures
		mpAccelData->SceneNode_BuildAccelData(&changedScenes);

		return true;
	}
	else
		return false;
}

void SceneLoader::BuildNodeIdMap()
{
	UINT32 sceneCnt = mpScene->GetKDSceneCnt();
	for (UINT32 si = 0; si < sceneCnt; ++si) {
		KScene* subScene = mpScene->GetKDScene(si);
		for (UINT32 ni = 0; ni < subScene->GetNodeCnt(); ++ni) {
			NodeId id = {si, ni};
			mNodeIDs[subScene->GetNode(ni)->mName] = id;
		}
	}
}


}