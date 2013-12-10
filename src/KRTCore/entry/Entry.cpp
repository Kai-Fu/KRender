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

bool InitializeKRayTracer()
{
	FreeImage_Initialise();
	KEnvShader::Initialize();

	g_pRoot = new KRayTracer_Root();
	return true;
}

void DestroyKRayTracer()
{
	if (g_pRoot)
		delete g_pRoot;
	g_pRoot = NULL;

	KEnvShader::Shutdown();
	FreeImage_DeInitialise();
}

KRayTracer_Root::KRayTracer_Root()
{
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

	char* tri_ray_hit = 
"void RayIntersectStaticTriArray(float% ray_org[], float% ray_dir[], float% tri_pos[], int% tri_id[], float% tuv[], int% hit_idx[], int cnt, int excluding_tri_id)\n"
"{\n"
"	int pos_idx = 0;\n"
"	tuv[0] = 3.402823466e+38F;\n"
"	for (int tri_i = 0; tri_i < cnt; tri_i = tri_i+1) {\n"

"		pos_idx = tri_i * 9;\n"
"		bool is_valid;\n"
"		float edge1[3], edge2[3], tvec[3], pvec[3], qvec[3], nface[3];\n"
"		float det,inv_det;\n"

"		is_valid = (tri_id[tri_i] != excluding_tri_id);\n"
		// find vectors for two edges sharing vert0 
		//SUB(edge1, vert1, vert0);
"		edge1[0] = tri_pos[pos_idx+3] - tri_pos[pos_idx+0];\n"
"		edge1[1] = tri_pos[pos_idx+4] - tri_pos[pos_idx+1];\n"
"		edge1[2] = tri_pos[pos_idx+5] - tri_pos[pos_idx+2];\n"
"		//SUB(edge2, vert2, vert0);\n"
"		edge2[0] = tri_pos[pos_idx+6] - tri_pos[pos_idx+0];\n"
"		edge2[1] = tri_pos[pos_idx+7] - tri_pos[pos_idx+1];\n"
"		edge2[2] = tri_pos[pos_idx+8] - tri_pos[pos_idx+2];\n"

		// begin calculating determinant - also used to calculate U parameter
		//CROSS(pvec, dir, edge2);
"		pvec[0] = ray_dir[1]*edge2[2] - ray_dir[2]*edge2[1];\n"
"		pvec[1] = ray_dir[2]*edge2[0] - ray_dir[0]*edge2[2];\n"
"		pvec[2] = ray_dir[0]*edge2[1] - ray_dir[1]*edge2[0];\n"
		//CROSS(nface, edge1, edge2);
"		nface[0] = edge1[1]*edge2[2] - edge1[2]*edge2[1];\n"
"		nface[1] = edge1[2]*edge2[0] - edge1[0]*edge2[2];\n"
"		nface[2] = edge1[0]*edge2[1] - edge1[1]*edge2[0];\n"

"		float dot_nd = nface[0]*ray_dir[0] + nface[1]*ray_dir[1] + nface[2]*ray_dir[2];\n"
"		is_valid = is_valid && (dot_nd <= 0);\n"// Backward face, ignore it

		// if determinant is near zero, ray lies in plane of triangle
"		//det = DOT(edge1, pvec);\n"
"		det = edge1[0]*pvec[0] + edge1[1]*pvec[1] + edge1[2]*pvec[2];\n"

"		is_valid = (det <= -0.000001 || det >= 0.000001) && is_valid;\n"
"		inv_det = 1.0f / det;\n"

		// calculate distance from vert0 to ray origin 
		//SUB(tvec, orig, vert0);
"		tvec[0] = ray_org[0] - tri_pos[pos_idx+0];\n"
"		tvec[1] = ray_org[1] - tri_pos[pos_idx+1];\n"
"		tvec[2] = ray_org[2] - tri_pos[pos_idx+2];\n"

		// calculate U parameter and test bounds
		//*u = DOT(tvec, pvec) * inv_det;
"		float tmpU = (tvec[0]*pvec[0] + tvec[1]*pvec[1] + tvec[2]*pvec[2]) * inv_det;\n"
"		is_valid = (tmpU >= 0.0 && tmpU <= 1.0) && is_valid;\n"

		// prepare to test V parameter
		//CROSS(qvec, tvec, edge1);
"		qvec[0] = tvec[1]*edge1[2] - tvec[2]*edge1[1];\n"
"		qvec[1] = tvec[2]*edge1[0] - tvec[0]*edge1[2];\n"
"		qvec[2] = tvec[0]*edge1[1] - tvec[1]*edge1[0];\n"

		// calculate V parameter and test bounds
		//*v = DOT(dir, qvec) * inv_det;
"		float tmpV = (ray_dir[0]*qvec[0] + ray_dir[1]*qvec[1] + ray_dir[2]*qvec[2]) * inv_det;\n"
"		is_valid = (tmpV >= 0.0 && tmpU + tmpV <= 1.0) && is_valid;\n"

		// calculate t, ray intersects triangle
"		float tmpT = (edge2[0]*qvec[0] + edge2[1]*qvec[1] + edge2[2]*qvec[2]) * inv_det;\n"
"		is_valid = (tmpT > 0) && is_valid;\n"

"		tuv[0] = is_valid ? tmpT : tuv[0];\n"
"		tuv[1] = is_valid ? tmpU : tuv[1];\n"
"		tuv[2] = is_valid ? tmpV : tuv[2];\n"
"		hit_idx[0] = is_valid ? tri_id[tri_i] : hit_idx[0];\n"
"	}\n"
"}\n"

"void RayIntersectAnimTriArray(float% ray_org[], float% ray_dir[], float cur_t, float% tri_pos[], int% tri_id[], float% tuv[], int% hit_idx[], int cnt, int excluding_tri_id)\n"
"{\n"
"	int pos_idx = 0;\n"
"	int tuv_idx = 0;\n"
"	for (int tri_i = 0; tri_i < cnt; tri_i = tri_i+1) {\n"
"		pos_idx = tri_i * 18;\n"
"		tuv_idx = tri_i * 3;\n"
"		tuv[tuv_idx+0] = 3.402823466e+38F;\n"
"		bool is_valid;\n"
"		float edge1[3], edge2[3], tvec[3], pvec[3], qvec[3], nface[3];\n"
"		float det,inv_det;\n"

"		is_valid = (tri_id[tri_i] != excluding_tri_id);\n"
"		float vert0[3];\n"
"		vert0[0] = tri_pos[pos_idx+0] + tri_pos[pos_idx+9 ]*cur_t;\n"
"		vert0[1] = tri_pos[pos_idx+1] + tri_pos[pos_idx+10]*cur_t;\n"
"		vert0[2] = tri_pos[pos_idx+2] + tri_pos[pos_idx+11]*cur_t;\n"
		// find vectors for two edges sharing vert0 
		//SUB(edge1, vert1, vert0);
"		edge1[0] = tri_pos[pos_idx+3] + tri_pos[pos_idx+3+9]*cur_t - vert0[0];\n"
"		edge1[1] = tri_pos[pos_idx+4] + tri_pos[pos_idx+4+9]*cur_t - vert0[1];\n"
"		edge1[2] = tri_pos[pos_idx+5] + tri_pos[pos_idx+5+9]*cur_t - vert0[2];\n"
		//SUB(edge2, vert2, vert0);
"		edge2[0] = tri_pos[pos_idx+6] + tri_pos[pos_idx+6+9]*cur_t - vert0[0];\n"
"		edge2[1] = tri_pos[pos_idx+7] + tri_pos[pos_idx+7+9]*cur_t - vert0[1];\n"
"		edge2[2] = tri_pos[pos_idx+8] + tri_pos[pos_idx+8+9]*cur_t - vert0[2];\n"

		// begin calculating determinant - also used to calculate U parameter
"		//CROSS(pvec, dir, edge2);\n"
"		pvec[0] = ray_dir[1]*edge2[2] - ray_dir[2]*edge2[1];\n"
"		pvec[1] = ray_dir[2]*edge2[0] - ray_dir[0]*edge2[2];\n"
"		pvec[2] = ray_dir[0]*edge2[1] - ray_dir[1]*edge2[0];\n"
		//CROSS(nface, edge1, edge2);
"		nface[0] = edge1[1]*edge2[2] - edge1[2]*edge2[1];\n"
"		nface[1] = edge1[2]*edge2[0] - edge1[0]*edge2[2];\n"
"		nface[2] = edge1[0]*edge2[1] - edge1[1]*edge2[0];\n"

"		float dot_nd = nface[0]*ray_dir[0] + nface[1]*ray_dir[1] + nface[2]*ray_dir[2];\n"
"		is_valid = is_valid && (dot_nd <= 0);\n"// Backward face, ignore it

		// if determinant is near zero, ray lies in plane of triangle
		//det = DOT(edge1, pvec);
"		det = edge1[0]*pvec[0] + edge1[1]*pvec[1] + edge1[2]*pvec[2];\n"

"		is_valid = (det <= -0.000001 || det >= 0.000001) && is_valid;\n"
"		inv_det = 1.0f / det;\n"

		// calculate distance from vert0 to ray origin
		//SUB(tvec, orig, vert0);
"		tvec[0] = ray_org[0] - vert0[0];\n"
"		tvec[1] = ray_org[1] - vert0[1];\n"
"		tvec[2] = ray_org[2] - vert0[2];\n"

		// calculate U parameter and test bounds
		//*u = DOT(tvec, pvec) * inv_det;
"		float tmpU = (tvec[0]*pvec[0] + tvec[1]*pvec[1] + tvec[2]*pvec[2]) * inv_det;\n"
"		is_valid = (tmpU >= 0.0 && tmpU <= 1.0) && is_valid;\n"

		// prepare to test V parameter
		//CROSS(qvec, tvec, edge1);
"		qvec[0] = tvec[1]*edge1[2] - tvec[2]*edge1[1];\n"
"		qvec[1] = tvec[2]*edge1[0] - tvec[0]*edge1[2];\n"
"		qvec[2] = tvec[0]*edge1[1] - tvec[1]*edge1[0];\n"

		// calculate V parameter and test bounds
		//*v = DOT(dir, qvec) * inv_det;
"		float tmpV = (ray_dir[0]*qvec[0] + ray_dir[1]*qvec[1] + ray_dir[2]*qvec[2]) * inv_det;\n"
"		is_valid = (tmpV >= 0.0 && tmpU + tmpV <= 1.0) && is_valid;\n"

		// calculate t, ray intersects triangle
"		float tmpT = (edge2[0]*qvec[0] + edge2[1]*qvec[1] + edge2[2]*qvec[2]) * inv_det;\n"
"		is_valid = (tmpT > 0) && is_valid;\n"

"		tuv[tuv_idx+0] = is_valid ? tmpT : tuv[tuv_idx+0];\n"
"		tuv[tuv_idx+1] = is_valid ? tmpU : tuv[tuv_idx+1];\n"
"		tuv[tuv_idx+2] = is_valid ? tmpV : tuv[tuv_idx+2];\n"
"		hit_idx[0] = is_valid ? tri_id[tri_i] : hit_idx[0];\n"

"	}\n"
"}\n"
;
	KSC_AddExternalFunction("_Sample2D", KSC_ShaderWithTexture::Sample2D);
	KSC_AddExternalFunction("_CalcSecondaryRay", _CalcSecondaryRay);
	KSC_AddExternalFunction("GetNextLightSample", _GetNextLightSample);
	bool ret = KSC_Initialize(predefines);
	if (ret) {
		KRayTracer::InitializeKRayTracer();
		ModuleHandle hTriRay = KSC_Compile(tri_ray_hit);
		if (hTriRay) {
			FunctionHandle hRayIntersectStaticTriArray = KSC_GetFunctionHandleByName("RayIntersectStaticTriArray", hTriRay);
			if (hRayIntersectStaticTriArray) {
				void* pFuncTriRay = KSC_GetFunctionPtr(hRayIntersectStaticTriArray);
				KAccelStruct_KDTree::s_pPFN_RayIntersectStaticTriArray = (KAccelStruct_KDTree::PFN_RayIntersectStaticTriArray)pFuncTriRay;
			}

			FunctionHandle hRayIntersectAnimTriArray = KSC_GetFunctionHandleByName("RayIntersectAnimTriArray", hTriRay);
			if (hRayIntersectAnimTriArray) {
				void* pFuncTriRay = KSC_GetFunctionPtr(hRayIntersectAnimTriArray);
				KAccelStruct_KDTree::s_pPFN_RayIntersectAnimTriArray = (KAccelStruct_KDTree::PFN_RayIntersectAnimTriArray)pFuncTriRay;
			}
		}

		if (KAccelStruct_KDTree::s_pPFN_RayIntersectStaticTriArray == NULL || 
			KAccelStruct_KDTree::s_pPFN_RayIntersectAnimTriArray == NULL)
			ret = false;
		else {
			// This is the urgly way to detect if certain SIMD instruction set is supported on current machine
			int alloc_size, alignment;
			KSC_GetBuiltInTypeInfo(SC::VarType::kBoolean, alloc_size, alignment);
			if (alloc_size == alignment) {
				// AVX support?
				KAccelStruct_KDTree::s_bSupportAVX = true;
			}
			else {
				KSC_GetBuiltInTypeInfo(SC::VarType::kFloat4, alloc_size, alignment);
				if (alloc_size == alignment) {
					// SSE support?
					KAccelStruct_KDTree::s_bSupportSSE = true;
				}
			}
		}
	}
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