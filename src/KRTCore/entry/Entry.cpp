#include "Entry.h"
#include "Constants.h"
#include "../util/HelperFunc.h"
#include "../camera/camera_manager.h"
#include "../material/material_library.h"
#include "../base/raw_geometry.h"
#include "../shader/environment_shader.h"
#include "../api/KRT_API.h"
#include <KShaderCompiler/inc/SC_API.h>

#include <FreeImage.h>

extern UINT32 AREA_LIGHT_SAMP_CNT;

namespace KRayTracer {

static KRayTracer_Root* g_pRoot = NULL;

KRayTracer_Root* InitializeKRayTracer()
{
	FreeImage_Initialise();

	KMaterialLibrary::Initialize();
	CameraManager::Initialize();
	LightScheme::Initialize();
	KEnvShader::Initialize();

	g_pRoot = new KRayTracer_Root();
	return g_pRoot;
}

void DestroyKRayTracer()
{
	KEnvShader::Shutdown();
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

bool KRayTracer_Root::UpdateTime(double timeInSec, double duration)
{
	if (mpSceneLoader.get() && mpSceneLoader->UpdateTime(timeInSec, duration))
		return true;
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

static void _CalcSecondaryRay(SurfaceContext::TracingData* pData, const KVec3* ray_dir, KColor* outClr)
{
	CalcSecondaryRay(pData->tracing_inst, pData->shading_ctx->position, pData->shading_ctx->excluding_bbox, pData->shading_ctx->excluding_tri, *ray_dir, *outClr);
}

static SC::Boolean _GetNextLightSample(SurfaceContext::TracingData* pData, KVec3* outLightDir, KVec3* outLightIntensity)
{
	LightScheme* pLightScheme = LightScheme::GetInstance();
	UINT32 lightCnt = pLightScheme->GetLightCount();

	while (1) {
		if (pData->iter_light_li >= lightCnt)
			break;

		UINT32 lightIdx = pData->iter_light_li;
		const ILightObject* pLight = pLightScheme->GetLightPtr(lightIdx);
		KVec2 sampPos(0, 0);
		float intensityScale = 1.0f;

		if (pLight->IsAreaLight()) {
			sampPos = pData->tracing_inst->GetAreaLightSample(pData->iter_light_li, pData->iter_light_si);
			intensityScale = 1.0f / AREA_LIGHT_SAMP_CNT;
			pData->iter_light_si++; // Move to next sample
			if (pData->iter_light_si >= AREA_LIGHT_SAMP_CNT) {
				pData->iter_light_si = 0;
				pData->iter_light_li++; // The sampling for this light is done, move to next light
			}
		}
		else
			pData->iter_light_li++; // Move to next light
	
		LightIterator li_it;
		bool res = pLightScheme->GetLightIter(pData->tracing_inst, sampPos, lightIdx, pData->shading_ctx, pData->hit_ctx, li_it);
		if (res) {
			*outLightDir = li_it.direction;
			(*outLightIntensity)[0] = li_it.intensity.r * intensityScale;
			(*outLightIntensity)[1] = li_it.intensity.g * intensityScale;
			(*outLightIntensity)[2] = li_it.intensity.b * intensityScale;
			return 1;
		}
	}

	return 0;
}

bool KRT_Initialize()
{
	char* predefines = 
"extern TracerData;\n"
"struct SurfaceContext\n"
"{\n"
"	float3 outVec;\n"

"	float3 normal;\n"
"	float3 tangent;\n"
"	float3 binormal;\n"

"	float2 uv;\n"

"	TracerData tracerData;"
"};\n"

"struct TransContext\n"
"{\n"
"	float3 lightVec;\n"

"	float3 normal;\n"
"	float3 tangent;\n"
"	float3 binormal;\n"

"	float2 uv;\n"
"};\n"

"extern Texture2D;\n"
"void _Sample2D(Texture2D tex, float2& uv, float4& outSample);\n"
"float4 Sample2D(Texture2D tex, float2 uv)\n"
"{\n"
"	float4 ret;\n"
"	_Sample2D(tex, uv, ret);\n"
"	return ret;\n"
"}\n"

"void _CalcSecondaryRay(TracerData pData, float3& ray_dir, float3& outClr);\n"
"float3 CalcSecondaryRay(TracerData pData, float3 ray_dir)\n"
"{\n"
"	float3 ret;\n"
"	_CalcSecondaryRay(pData, ray_dir, ret);\n"
"	return ret;\n"
"}\n"

"bool GetNextLightSample(TracerData pData, float3& outLightDir, float3& outLightIntensity);\n"
;
	KSC_AddExternalFunction("_Sample2D", KSC_ShaderWithTexture::Sample2D);
	KSC_AddExternalFunction("_CalcSecondaryRay", _CalcSecondaryRay);
	KSC_AddExternalFunction("GetNextLightSample", _GetNextLightSample);
	bool ret = KSC_Initialize(predefines);
	if (ret)
		KRayTracer::InitializeKRayTracer();

	return ret;
}

void KRT_Destory()
{
	KRayTracer::DestroyKRayTracer();
	KSC_Destory();
}

bool KRT_LoadScene(const char* fileName, KRT_SceneStatistic& statistic)
{
	bool ret = KRayTracer::g_pRoot->LoadScene(fileName);
	if (ret)
		KRayTracer::g_pRoot->mpSceneLoader->mpAccelData->GetKDBuildTimeStatistics(statistic);

	return ret;
}

bool KRT_UpdateTime(double timeInSec, double duration)
{
	return KRayTracer::g_pRoot->UpdateTime(timeInSec, duration);
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
	KSceneSet* pScene = (KSceneSet*)scene;
	UINT32 newSceneIdx = 0;
	KScene* pSubScene = pScene->AddKDScene(newSceneIdx);
	return newSceneIdx;
}

unsigned KRT_AddNodeToScene(TopSceneHandle scene, unsigned sceneIdx, float* matrix)
{
	KSceneSet* pScene = (KSceneSet*)scene;
	UINT32 nodeIdx = pScene->SceneNode_Create(sceneIdx);
	pScene->SceneNodeTM_SetStaticNode(sceneIdx, *(KMatrix4*)matrix);
	return nodeIdx;
}

SubSceneHandle KRT_GetSubSceneByIndex(TopSceneHandle scene, unsigned idx)
{
	KSceneSet* pScene = (KSceneSet*)scene;
	return pScene->GetKDScene(idx);
}

void KRT_AddSubSceneNodeFrame(TopSceneHandle scene, unsigned nodeIdx, float* matrix)
{
	KSceneSet* pScene = (KSceneSet*)scene;
	pScene->SceneNodeTM_SetStaticNode(nodeIdx, *(KMatrix4*)matrix);
}

void KRT_ResetSubSceneNodeTransform(TopSceneHandle scene, unsigned nodeIdx)
{
	KSceneSet* pScene = (KSceneSet*)scene;
	pScene->SceneNodeTM_SetStaticNode(nodeIdx, nvmath::cIdentity44f);
}

TopSceneHandle KRT_GetScene()
{
	return KRayTracer::g_pRoot->mpSceneLoader->mpScene;
}

static KNode* _GetSceneNode(const char* nodeName)
{
	if (KRayTracer::g_pRoot->mpSceneLoader.get()) {
		std::map<std::string, KRayTracer::SceneLoader::NodeId>::iterator it = KRayTracer::g_pRoot->mpSceneLoader->mNodeIDs.find(nodeName);
		if (it != KRayTracer::g_pRoot->mpSceneLoader->mNodeIDs.end()) {
			if (it->second.sceneIdx < KRayTracer::g_pRoot->mpSceneLoader->mpScene->GetKDSceneCnt()) {
				KScene* pScene = KRayTracer::g_pRoot->mpSceneLoader->mpScene->GetKDScene(it->second.sceneIdx);
				if (it->second.nodeIdx < pScene->GetNodeCnt()) {
					KNode* pNode = pScene->GetNode(it->second.nodeIdx);
					return pNode;
				}
			}
		}
	}
	return NULL;
}

ShaderHandle KRT_GetNodeSurfaceMaterial(const char* nodeName)
{
	KNode* pNode = _GetSceneNode(nodeName);
	if (pNode)
		return pNode->mpSurfShader;
	else
		return NULL;
}

void KRT_SetNodeSurfaceMaterial(const char* nodeName, ShaderHandle shader)
{
	KNode* pNode = _GetSceneNode(nodeName);
	if (pNode)
		pNode->mpSurfShader = (ISurfaceShader*)shader;
}