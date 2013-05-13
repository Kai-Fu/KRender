#include "Entry.h"
#include "Constants.h"
#include "../util/HelperFunc.h"
#include "../camera/camera_manager.h"
#include "../material/material_library.h"
#include "../base/raw_geometry.h"
#include "../material/standard_mtl.h"
#include "../api/KRT_API.h"
#include <KShaderCompiler/inc/SC_API.h>

#include <FreeImage.h>

namespace KRayTracer {

static KRayTracer_Root* g_pRoot = NULL;

KRayTracer_Root* InitializeKRayTracer()
{
	FreeImage_Initialise();

	KMaterialLibrary::Initialize();
	CameraManager::Initialize();
	LightScheme::Initialize();

	g_pRoot = new KRayTracer_Root();
	return g_pRoot;
}

void DestroyKRayTracer()
{
	LightScheme::Shutdown();
	CameraManager::Shutdown();
	KMaterialLibrary::Shutdown();

	if (g_pRoot)
		delete g_pRoot;
	g_pRoot = NULL;
	FreeImage_DeInitialise();
}

KRayTracer_Root::KRayTracer_Root()
{
	mpSceneLoader.reset(new SceneLoader());
}

KRayTracer_Root::~KRayTracer_Root()
{
}

bool KRayTracer_Root::SetConstant(const char* name, const char* value)
{
	if (mpTracingEntry.get()) {
		printf("Cannot change the constants when scene is loaded.\n");
		return false;
	}
	return SetGlobalConstant(name, value);
}

bool KRayTracer_Root::LoadScene(const char* filename)
{
	mpSceneLoader.reset(NULL);
	mpSceneLoader.reset(new KRayTracer::SceneLoader());
	if (mpSceneLoader->LoadFromFile(filename)) {

		return true;
	}
	else
		return false;
}

bool KRayTracer_Root::LoadUpdateFile(const char* filename)
{
	if (mpSceneLoader.get() && mpSceneLoader->LoadUpdatingFile(filename))
		return true;
	else
		return false;
}

bool KRayTracer_Root::SaveScene(const char* filename)
{
	if (mpSceneLoader.get())
		return mpSceneLoader->SaveToFile(filename);
	else
		return false;
}

void KRayTracer_Root::CloseScene()
{
	mpSceneLoader.reset(NULL);
}

const BitmapObject* KRayTracer_Root::Render(UINT32 w, UINT32 h, KRT_ImageFormat destFormat, void* pUserBuf, double& render_time)
{
	if (mpTracingEntry.get() == NULL)
		mpTracingEntry.reset(new SamplingThreadContainer);

	RenderParam param;
	param.is_refining = false;
	param.want_motion_blur = false;
	param.want_depth_of_field = false;
	param.want_global_illumination = false;
	param.want_edge_sampling = true;
	param.sample_cnt_eval = PIXEL_SAMPLE_CNT_EVAL;
	param.sample_cnt_more = PIXEL_SAMPLE_CNT_MORE;
	param.sample_cnt_edge = PIXEL_SAMPLE_CNT_EDGE;

	param.image_width = w;
	param.image_height = h;
	param.user_buffer = pUserBuf;
	param.pixel_format = destFormat;

	KTimer stop_watch(true);
		
	bool res = mpTracingEntry->Render(param, &mEventCB, NULL, mpSceneLoader.get());
	render_time = stop_watch.Stop();

	if (res) 
		return mpTracingEntry->mRenderBuffers.GetOutputImagePtr();
	else
		return NULL;
}

void KRayTracer_Root::SetCamera(const char* name, float pos[3], float lookat[3], float up_vec[3], float xfov)
{
	KCamera* pCamera = CameraManager::GetInstance()->GetCameraByName(name);
	if (!pCamera) {
		pCamera = CameraManager::GetInstance()->OpenCamera(name, true);
	}
		
	KVec3 cpos(pos[0], pos[1], pos[2]);
		
	KVec3 clookat(lookat[0],lookat[1], lookat[2]);
		
	KVec3 cup(up_vec[0], up_vec[1], up_vec[2]);
	KCamera::MotionState ms;
	ms.pos = cpos;
	ms.lookat = clookat;
	ms.up = cup;
	ms.xfov = xfov;
	ms.focal = 40; // TODO: avoid hard-coded value
	pCamera->SetupStillCamera(ms);
	pCamera->SetApertureSize(0.3f, 0.3f); // TODO: avoid hard-coded value
}

void KRayTracer_Root::EventNotifier::OnTileFinished(UINT32 sx, UINT32 sy, UINT32 w, UINT32 h)
{

}

void KRayTracer_Root::EventNotifier::OnFrameFinished(bool bIsUserCancel)
{
	mFrameFinishMutex.Signal();
}


} // namespace KRayTracer

bool KRT_Initialize()
{
	KRayTracer::InitializeKRayTracer();
	char* predefines = 
"struct SurfaceContext\n"
"{\n"
"	float3 inLight;\n"

"	float3 inVec;\n"
"	float3 outVec;\n"

"	float3 normal;\n"
"	float3 tangent;\n"
"	float3 binormal;\n"

"	float2 uv;\n"
"	};\n"

"extern Texture2D;\n"
"void _Sample2D(Texture2D& tex, float2& uv, float4& outSample);\n"

"float4 Sample2D(Texture2D& tex, float2 uv)\n"
"{\n"
"	float4 ret;\n"
"	_Sample2D(tex, uv, ret);\n"
"	return ret;\n"
"}\n"
;
	KSC_AddExternalFunction("_Sample2D", KSC_ShaderWithTexture::Sample2D);
	bool ret = KSC_Initialize(predefines);

	return ret;
}

void KRT_Destory()
{
	KSC_Destory();
	KRayTracer::DestroyKRayTracer();
}

bool KRT_LoadScene(const char* fileName, KRT_SceneStatistic& statistic)
{
	bool ret = KRayTracer::g_pRoot->LoadScene(fileName);
	if (ret)
		KRayTracer::g_pRoot->mpSceneLoader->mpScene->GetKDBuildTimeStatistics(statistic);

	return ret;
}

bool KRT_LoadUpdate(const char* fileName)
{
	return KRayTracer::g_pRoot->LoadUpdateFile(fileName);
}

bool KRT_SaveScene(const char* fileName)
{
	return KRayTracer::g_pRoot->SaveScene(fileName);
}

void KRT_CloseScene()
{
	KRayTracer::g_pRoot->CloseScene();
}

bool KRT_SetConstant(const char* name, const char* value)
{
	return KRayTracer::g_pRoot->SetConstant(name, value);
}

bool KRT_SetRenderOption(const char* name, const char* value)
{
	return SetRenderOptions(name, value);
}

bool KRT_RenderToMemory(unsigned w, unsigned h, KRT_ImageFormat format, void* pOutData, KRT_RenderStatistic& outStatistic)
{
	const void* renderData = KRayTracer::g_pRoot->Render(w, h, format, pOutData, outStatistic.render_time);
	BitmapObject bmpOrg;
	bmpOrg.mAutoFreeMem = false;
	bmpOrg.mFormat = BitmapObject::eRGB32F;
	bmpOrg.mWidth = w;
	bmpOrg.mHeight = h;
	bmpOrg.mpData = (BYTE*)renderData;


	BitmapObject bmpDest;
	bmpDest.mAutoFreeMem = false;
	bmpDest.mWidth = w;
	bmpDest.mHeight = h;

	switch (format) {
	case kRGB_8:
		bmpDest.mFormat = BitmapObject::eRGB8;
		bmpDest.mpData = (BYTE*)pOutData;
	}

	bmpDest.CopyFromRGB32F(bmpOrg, 0, 0, 0, 0, w, h);
	
	return false;
}

bool KRT_RenderToImage(unsigned w, unsigned h, KRT_ImageFormat format, const char* fileName, KRT_RenderStatistic& outStatistic)
{
	const BitmapObject* outBitmap = KRayTracer::g_pRoot->Render(w, h, kRGB_8, NULL, outStatistic.render_time);

	if (outBitmap) {
		outBitmap->Save(fileName);
		return true;
	}
	else
		return false;
}

bool KRT_SetCamera(const char* cameraName, float pos[3], float lookat[3], float up_vec[3], float xfov)
{
	KRayTracer::g_pRoot->SetCamera(cameraName, pos, lookat, up_vec, xfov);
	return true;
}

bool KRT_SetActiveCamera(const char* cameraName)
{
	CameraManager* pManager = CameraManager::GetInstance();
	if (!pManager) {
		printf("No scene is loaded.\n");
		return 0;
	}

	return pManager->SetActiveCamera(cameraName);
}

unsigned KRT_GetCameraCount()
{
	CameraManager* pManager = CameraManager::GetInstance();
	return pManager->GetCameraCnt();
}

const char* KRT_GetCameraName(unsigned idx)
{
	CameraManager* pManager = CameraManager::GetInstance();
	return pManager->GetCameraNameByIndex(idx);
}

unsigned KRT_AddLightSource(const char* shaderName, float matrix[16], float intensity[3])
{
	PointLightBase* pLight = dynamic_cast<PointLightBase*>(LightScheme::GetInstance()->CreateLightSource(POINT_LIGHT_TYPE));
	KMatrix4& mat = *(KMatrix4*)matrix;
	pLight->SetLightSpaceMatrix(mat);

	pLight->SetLightSpaceMatrix(mat);
	pLight->SetIntensity(intensity);
	return true;
}

void KRT_DeleteAllLights()
{
	LightScheme::GetInstance()->ClearLightSource();
}

unsigned KRT_AddMeshToSubScene(Geom::RawMesh* pMesh, SubSceneHandle subScene)
{
	KScene* pScene = (KScene*)subScene;
	UINT32 meshIdx = pScene->AddMesh();
	Geom::CompileOptimizedMesh(*pMesh, *pScene->GetMesh(meshIdx));

	return meshIdx;
}

ShaderHandle KRT_CreateSurfaceMaterial(const char* shaderName, const char* mtlName)
{
	KMaterialLibrary* mtl_lib = KMaterialLibrary::GetInstance();
	ISurfaceShader* pSS = mtl_lib->CreateMaterial(shaderName, mtlName);
	return pSS;
}

bool KRT_SetShaderParameter(ShaderHandle hShader, const char* paramName, void* valueData, unsigned dataSize)
{
	ISurfaceShader* pSS = (ISurfaceShader*)hShader;
	pSS->SetParam(paramName, valueData, dataSize);
	return true;
}

ShaderHandle KRT_GetSurfaceMaterial(const char* mtlName)
{
	KMaterialLibrary* mtl_lib = KMaterialLibrary::GetInstance();
	ISurfaceShader* pSurfShader = mtl_lib->OpenMaterial(mtlName);
	return pSurfShader;
}

void KRT_AddNodeToSubScene(SubSceneHandle subScene, const char* nodeName, unsigned meshIdx, ShaderHandle hShader, float* matrix)
{
	KScene* pScene = (KScene*)subScene;
	UINT32 nodeIdx = pScene->AddNode();
	KNode* pNode = pScene->GetNode(nodeIdx);
	pNode->mMesh.push_back((UINT32)meshIdx);
	pNode->mpSurfShader = (ISurfaceShader*)hShader;
	pNode->mName = nodeName;
	KMatrix4& nodeTM = *(KMatrix4*)matrix;
	pScene->SetNodeTM(nodeIdx, nodeTM);
}

unsigned KRT_AddSubScene(TopSceneHandle scene)
{
	KKDBBoxScene* pScene = (KKDBBoxScene*)scene;
	UINT32 newSceneIdx = 0;
	KKDTreeScene* pSubScene = pScene->AddKDScene(newSceneIdx);
	return newSceneIdx;
}

unsigned KRT_AddNodeToScene(TopSceneHandle scene, unsigned sceneIdx, float* matrix)
{
	KKDBBoxScene* pScene = (KKDBBoxScene*)scene;
	UINT32 nodeIdx = pScene->SceneNode_Create(sceneIdx);
	pScene->SceneNodeTM_ResetFrame(sceneIdx);
	pScene->SceneNodeTM_AddFrame(sceneIdx, *(KMatrix4*)matrix);
	return nodeIdx;
}

SubSceneHandle KRT_GetSubSceneByIndex(TopSceneHandle scene, unsigned idx)
{
	KKDBBoxScene* pScene = (KKDBBoxScene*)scene;
	return pScene->GetKDScene(idx);
}

void KRT_AddSubSceneNodeFrame(TopSceneHandle scene, unsigned nodeIdx, float* matrix)
{
	KKDBBoxScene* pScene = (KKDBBoxScene*)scene;
	pScene->SceneNodeTM_AddFrame(nodeIdx, *(KMatrix4*)matrix);
}

void KRT_ResetSubSceneNodeTransform(TopSceneHandle scene, unsigned nodeIdx)
{
	KKDBBoxScene* pScene = (KKDBBoxScene*)scene;
	pScene->SceneNodeTM_ResetFrame(nodeIdx);
}

TopSceneHandle KRT_GetScene()
{
	return KRayTracer::g_pRoot->mpSceneLoader->mpScene;
}

bool KRT_SaveUpdate(const char* file_name, 
			const std::vector<UINT32>& modified_cameras,
			const std::vector<UINT32>& modified_lights,
			const std::vector<UINT32>& modified_morph_nodes, 
			const std::vector<UINT32>& modified_RBA_nodes)
{

	return KRayTracer::g_pRoot->mpSceneLoader->SaveUpdatingFile(file_name, 
				modified_cameras,
				modified_lights,
				modified_morph_nodes,
				modified_RBA_nodes);
}