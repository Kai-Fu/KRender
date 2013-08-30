#include "shader_api.h"
#include "../util/HelperFunc.h"
#include "../sampling/halton2d.h"
#include "../animation/animated_transform.h"
#include "../scene/KKDBBoxScene.h"
#include "../image/basic_map.h"

#include <assert.h>

#define RAND_SEQUENCE_LEN 1023
extern UINT32 AREA_LIGHT_SAMP_CNT;

void RenderBuffers::SetImageSize(UINT32 w, UINT32 h, KRT_ImageFormat pixelFormat, void* pUserBuf)
{
	switch (pixelFormat) {
	case kRGB_8:
		output_image.reset(BitmapObject::CreateBitmap(w, h, BitmapObject::eRGB8, pUserBuf));
		break;
	case kRGBA_8:
		output_image.reset(BitmapObject::CreateBitmap(w, h, BitmapObject::eRGBA8, pUserBuf));
		break;
	case kRGBA_32F:
		output_image.reset(BitmapObject::CreateBitmap(w, h, BitmapObject::eRGBA32F, pUserBuf));
		break;
	case kRGB_32F:
		output_image.reset(BitmapObject::CreateBitmap(w, h, BitmapObject::eRGB32F, pUserBuf));
		break;
	case kR_32F:
		output_image.reset(BitmapObject::CreateBitmap(w, h, BitmapObject::eR32F, pUserBuf));
		break;
	}
	sampled_count_pp.resize(w * h);
	random_seed_pp.resize(w * h);
	for (UINT32 i = 0; i < random_seed_pp.size(); ++i) {
		sampled_count_pp[i] = 0;
		random_seed_pp[i] = (UINT32)rand() % RAND_SEQUENCE_LEN;
	}

	for (UINT32 y = 0; y < h; ++y) {
		void *pPixel = output_image->GetPixelPtr(0, y);
		memset(pPixel, 0, output_image->mPitch);
	}

	random_sequence.resize(RAND_SEQUENCE_LEN);
	for (UINT32 i = 0; i < RAND_SEQUENCE_LEN; ++i)
		random_sequence[i] = Rand_0_1();
}

void RenderBuffers::AddSamples(UINT32 x, UINT32 y, UINT32 sampleCnt, const KColor& avgClr, float alpha)
{
	KPixel curPixel = output_image->GetPixel(x, y);
	UINT32 sampled_count = GetSampledCount(x, y);
	float fSampledCnt = (float)sampled_count - sampleCnt;
	float fSampleCnt = (float)sampleCnt;

	KColor tempClr0 = curPixel.color;
	tempClr0.Scale(fSampledCnt);
	KColor tempClr1(avgClr);
	tempClr1.Scale(fSampleCnt);
	tempClr0.Add(tempClr1);
	tempClr0.Scale(1.0f / (fSampledCnt + fSampleCnt));
	curPixel.color = tempClr0;

	float newAlpha = (alpha*fSampleCnt  + curPixel.alpha*fSampledCnt) / (fSampledCnt + fSampleCnt);
	curPixel.alpha = newAlpha;
	output_image->SetPixel(x, y, curPixel);
}

UINT32 RenderBuffers::GetSampledCount(UINT32 x, UINT32 y) const
{
	return sampled_count_pp[y * output_image->mWidth + x];
}

void RenderBuffers::IncreaseSampledCount(UINT32 x, UINT32 y, UINT32 sampleCnt)
{
	sampled_count_pp[y * output_image->mWidth + x] += sampleCnt;
}

UINT32 RenderBuffers::GetRandomSeed(UINT32 x, UINT32 y) const
{
	return random_seed_pp[y * output_image->mWidth + x];
}

float RenderBuffers::GetPixelRandom(UINT32 x, UINT32 y, UINT32 offset) const
{
	UINT32 randSeed = GetRandomSeed(x, y);
	float res = random_sequence[(randSeed+offset) % RAND_SEQUENCE_LEN];
	return res;
}




#define RS_SCREEN_X_OFFSET	0
#define RS_SCREEN_Y_OFFSET	1
#define RS_DOF_X_OFFSET		2
#define RS_DOF_Y_OFFSET		3
#define RS_TIME_OFFSET		4
#define RS_LIGHT_OFFSET		5

float RenderBuffers::GetPixelRandom(UINT32 x, UINT32 y, UINT32 offset, float min, float max) const
{
	return nvmath::lerp(GetPixelRandom(x, y, offset), min, max);
}

KVec2 RenderBuffers::RS_Image(UINT32 x, UINT32 y) const
{
	KVec2 warp;
	float fx = (float)x;
	float fy = (float)y;
	KVec2 hPt;
	UINT32 sampledCnt = GetSampledCount(x, y);
	
	warp[0] = GetPixelRandom(x, y, RS_SCREEN_X_OFFSET);
	warp[1] = GetPixelRandom(x, y, RS_SCREEN_Y_OFFSET);

	KVec2 min(fx - 0.5f, fy - 0.5f);
	KVec2 max(fx + 0.5f, fy + 0.5f);

	Sampling::Halton2D(&hPt, sampledCnt, 1, warp, min, max);
	return hPt;
}

KVec2 RenderBuffers::RS_DOF(UINT32 x, UINT32 y, const KVec2& apertureSize) const
{
	KVec2 warp;
	float fx = (float)x;
	float fy = (float)y;
	KVec2 hPt;
	UINT32 sampledCnt = GetSampledCount(x, y);
	
	warp[0] = GetPixelRandom(x, y, RS_DOF_X_OFFSET);
	warp[1] = GetPixelRandom(x, y, RS_DOF_Y_OFFSET);

	KVec2 halfApertureSize = apertureSize;
	halfApertureSize *= 0.5f;
	KVec2 min(-halfApertureSize[0], halfApertureSize[0]);
	KVec2 max(-halfApertureSize[1], halfApertureSize[1]);

	Sampling::Halton2D(&hPt, sampledCnt, 1, warp, min, max);
	return hPt;
}

KVec2 RenderBuffers::RS_AreaLight(UINT32 x, UINT32 y, UINT32 lightIdx, UINT32 sampleIdx) const
{
	KVec2 warp;
	float fx = (float)x;
	float fy = (float)y;
	KVec2 hPt;
	UINT32 sampledCnt = GetSampledCount(x, y);
	UINT32 lightOffset = lightIdx * 2;

	warp[0] = GetPixelRandom(x, y, RS_LIGHT_OFFSET + lightOffset);
	warp[1] = GetPixelRandom(x, y, RS_LIGHT_OFFSET + 1 + lightOffset);

	KVec2 min(-1, -1);
	KVec2 max(1, 1);

	Sampling::Halton2D(&hPt, sampledCnt * AREA_LIGHT_SAMP_CNT + sampleIdx, 1, warp, min, max);
	return hPt;
}

float RenderBuffers::RS_MotionBlur(UINT32 x, UINT32 y) const
{
	KVec2 warp;
	float fx = (float)x;
	float fy = (float)y;
	KVec2 hPt;
	UINT32 sampledCnt = GetSampledCount(x, y);
	
	warp[0] = GetPixelRandom(x, y, RS_TIME_OFFSET);
	warp[1] = GetPixelRandom(x, y, RS_TIME_OFFSET);

	KVec2 min(0, 0);
	KVec2 max(1, 1);

	Sampling::Halton2D(&hPt, sampledCnt, 1, warp, min, max);
	return hPt[0];
}

const BitmapObject* RenderBuffers::GetOutputImagePtr() const
{
	return output_image.get();
}

TracingInstance::TracingInstance(const KAccelStruct_BVH* scene, const RenderBuffers* pBuffers) :
	mCachedTriPosData(MAX_REFLECTION_BOUNCE * 3 + 11)
{
	mpScene = scene;
	mpRenderBuffers = pBuffers;
	mCurPixel_X = INVALID_INDEX;
	mCurPixel_Y = INVALID_INDEX;
	mIsPixelSampling = false;
	mBounceDepth = 0;

	mSurfaceContexts.resize(MAX_REFLECTION_BOUNCE);
	mTransContexts.resize(MAX_REFLECTION_BOUNCE);
	KSC_TypeInfo surfaceCtxType = KSC_GetStructTypeByName("SurfaceContext", NULL);
	KSC_TypeInfo transCtxType = KSC_GetStructTypeByName("TransContext", NULL);
	for (UINT32 i = 0; i < MAX_REFLECTION_BOUNCE; ++i) {
		mSurfaceContexts[i].Allocate(surfaceCtxType);
		mTransContexts[i].Allocate(transCtxType);
	}
}

TracingInstance::~TracingInstance()
{

}

SurfaceContext& TracingInstance::GetCurrentSurfaceCtxStorage()
{
	return mSurfaceContexts[mBounceDepth - 1];
}

void TracingInstance::ConvertToTransContext(const IntersectContext& hitCtx, const ShadingContext& shadingCtx, TransContext& transCtx)
{
	*transCtx.lightVec = shadingCtx.out_vec;
	*transCtx.normal = shadingCtx.normal;
	*transCtx.tangent = shadingCtx.tangent.tangent;
	*transCtx.binormal = shadingCtx.tangent.binormal;
	*transCtx.uv = shadingCtx.hasUV ? shadingCtx.uv.uv : KVec2(0,0);
}

TransContext& TracingInstance::GetCurrentTransCtxStorage()
{
	return mTransContexts[mBounceDepth - 1];
}

const KAccelStruct_BVH* TracingInstance::GetScenePtr() const
{
	return mpScene;
}

KVec2 TracingInstance::GetAreaLightSample(UINT32 lightIdx, UINT32 sampleIdx) const
{
	if (mIsPixelSampling)
		return mpRenderBuffers->RS_AreaLight(mCurPixel_X, mCurPixel_Y, lightIdx, sampleIdx);
	else {
		KVec2 min(0, 0);
		KVec2 max(1, 1);
		KVec2 hPt;
		Sampling::Halton2D(&hPt, lightIdx*AREA_LIGHT_SAMP_CNT + sampleIdx, 1, KVec2(0,0), min, max);
		return KVec2(Rand_0_1(), Rand_0_1());
	}
}

void TracingInstance::SetCurrentPixel(UINT32 x, UINT32 y)
{
	mCurPixel_X = x;
	mCurPixel_Y = y;
	mIsPixelSampling = true;
}

void TracingInstance::IncBounceDepth()
{
	++mBounceDepth;
}

void TracingInstance::DecBounceDepth()
{
	assert(mBounceDepth > 0);
	--mBounceDepth;
}

UINT32 TracingInstance::GetBoundDepth() const
{
	return mBounceDepth;
}

KColor TracingInstance::GetBackGroundColor(const KVec3& dir) const
{
	return KColor(0,0,0);
}

void TracingInstance::CalcuShadingContext(const KRay& hitRay, const IntersectContext& hit_ctx, ShadingContext& out_shading_ctx) const
{
	const KSceneSet* pPlainScene = mpScene->GetSource();
	const KScene* pKDScene = pPlainScene->GetNodeKDScene(hit_ctx.bbox_node_idx);
	KAnimation::LocalTRSFrame::LclTRS nodeTRS;
	pPlainScene->GetNodeTransform(nodeTRS, hit_ctx.bbox_node_idx, mCameraContext.inMotionTime);
	const KQuat& scene_rot = nodeTRS.node_rot;
	const KTriDesc* pTri = mpScene->GetAccelTriData(hit_ctx.bbox_node_idx, hit_ctx.tri_id);
	UINT32 mesh_idx = pTri->GetMeshIdx();
	UINT32 node_idx = pTri->GetNodeIdx();
	UINT32 tri_idx = pTri->mTriIdx;
	const KTriMesh* pMesh = pKDScene->GetMesh(mesh_idx);
	const KNode* pNode = pKDScene->GetNode(node_idx);
	const UINT32* pn_idx = pMesh->mFaces[tri_idx].pn_idx;
	
	out_shading_ctx.tracing_instance = this;
	out_shading_ctx.excluding_bbox = hit_ctx.bbox_node_idx;
	out_shading_ctx.excluding_tri = hit_ctx.tri_id;
	assert(out_shading_ctx.excluding_bbox < NOT_HIT_INDEX);
	assert(out_shading_ctx.excluding_tri < NOT_HIT_INDEX);
	
	KTriMesh::PN_Data pn_vert[3];
	pMesh->ComputePN_Data(pn_vert[0], pMesh->mFaces[tri_idx].pn_idx[0], mCameraContext.inMotionTime);
	pMesh->ComputePN_Data(pn_vert[1], pMesh->mFaces[tri_idx].pn_idx[1], mCameraContext.inMotionTime);
	pMesh->ComputePN_Data(pn_vert[2], pMesh->mFaces[tri_idx].pn_idx[2], mCameraContext.inMotionTime);

	if (!pMesh->mTexFaces.empty()) {
		KTriMesh::TT_Data tt_data;
		pMesh->InterpolateTT(tri_idx, hit_ctx, tt_data, mCameraContext.inMotionTime);
		out_shading_ctx.uv.uv = tt_data.texcoord;
		out_shading_ctx.tangent.tangent = tt_data.tangent;
		out_shading_ctx.tangent.binormal = tt_data.binormal;
		out_shading_ctx.hasUV = 1;
	}
	else
		out_shading_ctx.hasUV = 0;

	KVec3 temp_vec;
	// interpolate the normal
	temp_vec =	pn_vert[0].nor * hit_ctx.w +
				pn_vert[1].nor * hit_ctx.u +
				pn_vert[2].nor * hit_ctx.v;
	out_shading_ctx.normal = temp_vec * pNode->GetObjectRot();
	temp_vec = out_shading_ctx.normal * scene_rot;
	out_shading_ctx.normal = temp_vec;
	float rcp_len_nor = 1.0f / nvmath::length(temp_vec);
	out_shading_ctx.normal *= rcp_len_nor;

	// interpolate the position
	out_shading_ctx.position = pn_vert[0].pos * hit_ctx.w +
						 pn_vert[1].pos * hit_ctx.u +
						 pn_vert[2].pos * hit_ctx.v;
	Vec3TransformCoord(temp_vec, out_shading_ctx.position, pNode->GetObjectTM());
	Vec3TransformCoord(out_shading_ctx.position, temp_vec, nodeTRS.node_tm);
	
	const nvmath::Quatf world_rot = pNode->GetObjectRot() * scene_rot;
	KVec3 edge[3];
	edge[0] = (pn_vert[1].pos - pn_vert[0].pos) * world_rot;
	edge[1] = (pn_vert[2].pos - pn_vert[1].pos) * world_rot;
	edge[2] = (pn_vert[0].pos - pn_vert[2].pos) * world_rot;
	float edge_len_sqr[3];
	edge_len_sqr[0] = nvmath::lengthSquared(edge[0]);
	edge_len_sqr[1] = nvmath::lengthSquared(edge[1]);
	edge_len_sqr[2] = nvmath::lengthSquared(edge[2]);
	float edge_len[3];
	edge_len[0] = sqrt(edge_len_sqr[0]);
	edge_len[1] = sqrt(edge_len_sqr[1]);
	edge_len[2] = sqrt(edge_len_sqr[2]);
	// Calculate the face normal and triangle size
	out_shading_ctx.face_size = edge_len[0] + edge_len[1] + edge_len[2];
	out_shading_ctx.face_size *= 0.05f;
	// Move the shading position along it normal for a epsilon distance.
	out_shading_ctx.position += (out_shading_ctx.normal * out_shading_ctx.face_size * 0.0001f);

	KVec3 face_nor;
	face_nor = edge[0] ^ edge[1];
	out_shading_ctx.face_normal = face_nor;
	out_shading_ctx.face_normal.normalize();

	KVec3d rayDir = hitRay.GetDir();
	rayDir.normalize(); // Normalize the ray direction because it's not normalized

	// Get the surface shader
	ISurfaceShader* pSurfShader = pNode->mpSurfShader;
	out_shading_ctx.surface_shader = pSurfShader;

	out_shading_ctx.out_vec = ToVec3f(-rayDir);
}

const ISurfaceShader* TracingInstance::GetSurfaceShader(const IntersectContext& hit_ctx) const
{
	const KSceneSet* pPlainScene = mpScene->GetSource();
	const KScene* pKDScene = pPlainScene->GetNodeKDScene(hit_ctx.bbox_node_idx);
	const KTriDesc* pTri = mpScene->GetAccelTriData(hit_ctx.bbox_node_idx, hit_ctx.tri_id);
	UINT32 node_idx = pTri->GetNodeIdx();
	const KNode* pNode = pKDScene->GetNode(node_idx);
	return pNode->mpSurfShader;
}

void TracingInstance::CalcuHitInfo(const IntersectContext& hit_ctx, IntersectInfo& out_info) const
{
	const KSceneSet* pPlainScene = mpScene->GetSource();
	const KScene* pKDScene = pPlainScene->GetNodeKDScene(hit_ctx.bbox_node_idx);
	KAnimation::LocalTRSFrame::LclTRS nodeTRS;
	pPlainScene->GetNodeTransform(nodeTRS, hit_ctx.bbox_node_idx, mCameraContext.inMotionTime);
	const KQuat& scene_rot = nodeTRS.node_rot;
	const KTriDesc* pTri = mpScene->GetAccelTriData(hit_ctx.bbox_node_idx, hit_ctx.tri_id);
	UINT32 mesh_idx = pTri->GetMeshIdx();
	UINT32 node_idx = pTri->GetNodeIdx();
	UINT32 tri_idx = pTri->mTriIdx;
	const KTriMesh* pMesh = pKDScene->GetMesh(mesh_idx);
	const KNode* pNode = pKDScene->GetNode(node_idx);
	const nvmath::Quatf world_rot = pNode->GetObjectRot() * scene_rot;

	KTriMesh::PN_Data pn_vert[3];
	pMesh->ComputePN_Data(pn_vert[0], pMesh->mFaces[tri_idx].pn_idx[0], mCameraContext.inMotionTime);
	pMesh->ComputePN_Data(pn_vert[1], pMesh->mFaces[tri_idx].pn_idx[1], mCameraContext.inMotionTime);
	pMesh->ComputePN_Data(pn_vert[2], pMesh->mFaces[tri_idx].pn_idx[2], mCameraContext.inMotionTime);

	out_info.pos = pn_vert[0].pos * hit_ctx.w +
		pn_vert[1].pos * hit_ctx.u +
		pn_vert[2].pos * hit_ctx.v;
	KVec3 temp_vec;
	Vec3TransformCoord(temp_vec, out_info.pos, pNode->GetObjectTM());
	Vec3TransformCoord(out_info.pos, temp_vec, nodeTRS.node_tm);

	// interpolate the normal
	temp_vec =	pn_vert[0].nor * hit_ctx.w +
		pn_vert[1].nor * hit_ctx.u +
		pn_vert[2].nor * hit_ctx.v;
	KVec3 normal = temp_vec * pNode->GetObjectRot();
	out_info.nor = normal * scene_rot;
	out_info.nor.normalize();

	KVec3 edge[3];
	edge[0] = (pn_vert[1].pos - pn_vert[0].pos);
	edge[1] = (pn_vert[2].pos - pn_vert[1].pos);
	KVec3 nor;
	nor = edge[0] ^ edge[1];
	nor.normalize();
	out_info.face_nor = nor * world_rot;

	out_info.bbox_node_idx = hit_ctx.bbox_node_idx;
	out_info.tri_id = hit_ctx.tri_id;
}
	
bool TracingInstance::CastRay(const KRay& ray, IntersectContext& out_ctx)
{
	if (mpScene->IntersectRay_KDTree(ray, this, out_ctx)) {

		return true;
	}
	else
		return false;
}

bool TracingInstance::IsPointOccluded(const KRay& ray, float len)
{
	IntersectContext ctx;
	if (CastRay(ray, ctx)) {
		double diff = ctx.ray_t - len;
		float epsilon = mpScene->GetSceneEpsilon();
		if (diff > epsilon)
			return false;
		else
			return true;
	}
	else
		return false;
}

SurfaceContext::SurfaceContext()
{
	mpData = NULL;

	outVec = NULL;
	
	normal = NULL;
	tangent = NULL;
	binormal = NULL;
	
	uv = NULL;
}

SurfaceContext::~SurfaceContext()
{
	if (mpData)
		KSC_FreeMem(mpData);
}

void SurfaceContext::Allocate(const KSC_TypeInfo& kscType)
{
	assert(kscType.hStruct);
	mpData = KSC_AllocMemForType(kscType, 1);
	outVec = (KVec3*)KSC_GetStructMemberPtr(kscType.hStruct, mpData, "outVec");

	normal = (KVec3*)KSC_GetStructMemberPtr(kscType.hStruct, mpData, "normal");
	tangent = (KVec3*)KSC_GetStructMemberPtr(kscType.hStruct, mpData, "tangent");
	binormal = (KVec3*)KSC_GetStructMemberPtr(kscType.hStruct, mpData, "binormal");
	
	uv = (KVec2*)KSC_GetStructMemberPtr(kscType.hStruct, mpData, "uv");

	tracerData = (TracingData**)KSC_GetStructMemberPtr(kscType.hStruct, mpData, "tracerData");
	*tracerData = &tracerDataLocal;
}

void TransContext::Allocate(const KSC_TypeInfo& kscType)
{
	assert(kscType.hStruct);
	mpData = KSC_AllocMemForType(kscType, 1);
	lightVec = (KVec3*)KSC_GetStructMemberPtr(kscType.hStruct, mpData, "lightVec");

	normal = (KVec3*)KSC_GetStructMemberPtr(kscType.hStruct, mpData, "normal");
	tangent = (KVec3*)KSC_GetStructMemberPtr(kscType.hStruct, mpData, "tangent");
	binormal = (KVec3*)KSC_GetStructMemberPtr(kscType.hStruct, mpData, "binormal");
	
	uv = (KVec2*)KSC_GetStructMemberPtr(kscType.hStruct, mpData, "uv");
}

void TracingInstance::ConvertToSurfaceContext(const IntersectContext& hitCtx, const ShadingContext& shadingCtx, SurfaceContext& surfaceCtx)
{
	surfaceCtx.tracerDataLocal.hit_ctx = &hitCtx;
	surfaceCtx.tracerDataLocal.shading_ctx = &shadingCtx;
	surfaceCtx.tracerDataLocal.tracing_inst = this;
	surfaceCtx.tracerDataLocal.iter_light_li = 0;
	surfaceCtx.tracerDataLocal.iter_light_si = 0;

	*surfaceCtx.outVec = shadingCtx.out_vec;
	*surfaceCtx.normal = shadingCtx.normal;
	*surfaceCtx.tangent = shadingCtx.tangent.tangent;
	*surfaceCtx.binormal = shadingCtx.tangent.binormal;
	*surfaceCtx.uv = shadingCtx.hasUV ? shadingCtx.uv.uv : KVec2(0,0);
}


std::hash_map<std::string, FunctionHandle> KSC_Shader::sLoadedShadeFunctions;

KSC_Shader::KSC_Shader()
{
	mpUniformData = NULL;
	mpFuncPtr = NULL;
	mShadeFunction = NULL;
}

KSC_Shader::KSC_Shader(const KSC_Shader& ref)
{
	mpFuncPtr = ref.mpFuncPtr;
	mShadeFunction = ref.mShadeFunction;
	mModifiedData = ref.mModifiedData;

	mUnifomArgType = ref.mUnifomArgType;
	mpUniformData = KSC_AllocMemForType(ref.mUnifomArgType, 0);

	assert(mUnifomArgType.hStruct);
	memcpy(mpUniformData, ref.mpUniformData, mUnifomArgType.sizeOfType);
}

bool KSC_Shader::LoadTemplate(const char* templateFile)
{
	printf("Loading shader template %s...\n", templateFile);

	mpUniformData = NULL;
	FILE* f = NULL;
	std::string templatePath = "./asset/";
	templatePath += templateFile;
	fopen_s(&f, templatePath.c_str(), "r");
	if (f == NULL)
		return false;
	char line[256];
	// The first line must be "Shader = xxx", where xxx should be the file that contains KSC code.
	fgets(line, 256, f);
	char kscFile[256];
	{
		char tempBuf0[256];
		if (1 != sscanf(line, "%*[^=]=%[^=]", tempBuf0)) {
			printf("\tIncorrect line for shader assignment: \"%s\" \n", line);
			return false;
		}
		sscanf(tempBuf0, "%s", kscFile);
	}

	FunctionHandle shadeFunc = NULL;
	ModuleHandle shaderModule = NULL;
	if (sLoadedShadeFunctions.find(kscFile) == sLoadedShadeFunctions.end()) {

		std::string shaderPath = "./asset/shader/";
		std::string shaderContent;
		shaderPath += kscFile;
		{
			FILE* fShadeFile = NULL;
			// Load the content of the shader file
			fopen_s(&fShadeFile, shaderPath.c_str(), "r");
			if (fShadeFile == NULL)
				return false;
			fseek(fShadeFile, 0, SEEK_END);
			long len = ftell(fShadeFile);
			fseek(fShadeFile, 0, SEEK_SET);

			shaderContent.resize(len + 1);
			char* line = &shaderContent[0];

			while (fgets(line, len, fShadeFile) != NULL) {
				size_t lineLen = strlen(line);
				line += lineLen;
			}
			fclose(fShadeFile);
		}

		shaderModule = KSC_Compile(shaderContent.c_str());
		if (shaderModule == NULL) {
			printf("Shader compilation failed with following error:\n");
			printf(KSC_GetLastErrorMsg());
			return false;
		}

		shadeFunc = KSC_GetFunctionHandleByName("Shade", shaderModule);
		if (shadeFunc == NULL) {
			printf("Shade function does not exist in KSC code.\n");
			return false;
		}
		sLoadedShadeFunctions[kscFile] = shadeFunc;
	}
	else {
		shadeFunc = sLoadedShadeFunctions[kscFile];
	}

	mpFuncPtr = KSC_GetFunctionPtr(shadeFunc);
	if (mpFuncPtr == NULL) {
		printf("Shade function JIT failed.\n");
		return false;
	}

	// The Shade function should have three arguments:
	// arg0 - the reference to structure that contains uniform data
	// arg1 - shader input data
	// arg2 - shader output data
	if (KSC_GetFunctionArgumentCount(shadeFunc) != 3) {
		printf("Incorrect argument count for Shade function.\n");
		return false;
	}

	KSC_TypeInfo arg0TypeInfo = KSC_GetFunctionArgumentType(shadeFunc, 0);
	if (!arg0TypeInfo.isRef || !arg0TypeInfo.isKSCLayout || arg0TypeInfo.hStruct == NULL) {
		printf("Incorrect type for argument 0.\n");
		return false;
	}
	for (int i = 1; i < 3; ++i) {
		KSC_TypeInfo typeInfo = KSC_GetFunctionArgumentType(shadeFunc, i);
		if (!typeInfo.isRef) {
			printf("Incorrect type for argument %d.\n", i);
			return false;
		}
	}
	mUnifomArgType = arg0TypeInfo;

	
	if (!HandleModule(shaderModule)) {
		printf("Failed to handle the compiled KSC module.\n");
		return false;
	}

	// Perform the additional check
	if (!Validate(shadeFunc)) {
		printf("Shade function validation failed.\n");
		return false;
	}

	mShadeFunction = shadeFunc;

	mpUniformData = KSC_AllocMemForType(arg0TypeInfo, 0);

	while (!feof(f)) {
		// Parse each line and set the initial uniform values
		fgets(line, 256, f);
		char varStr[256];
		char valueStr[256];
		{
			char tempBuf0[256];
			char tempBuf1[256];
			if (2 != sscanf(line, "%[^=]=%[^=]", tempBuf0, tempBuf1)) {
				printf("\tIncorrect line for shader parameter initializer: \"%s\" \n", line);
				continue;
			}
			sscanf(tempBuf0, "%s", varStr);
			sscanf(tempBuf1 + strspn(tempBuf1, " \t"), "%[^ ]", valueStr);
		}

		if (InitializeUniform(varStr))
			continue;  // This uniform is initialized by the derived class

		KSC_TypeInfo memberType = KSC_GetStructMemberType(arg0TypeInfo.hStruct, varStr);
		if (memberType.hStruct != NULL) {
			printf("\tShader parameter initializer must be applied to built-in types.\n");
			continue;
		}


		if (memberType.type == SC::kExternType) {
			void* extData = CreateExternalData(memberType.typeString, valueStr);
			if (extData == NULL) 
				printf("\tExternal data(type %s, value %s) creation failed.\n", memberType.typeString, valueStr);
			else {
				KSC_SetStructMemberData(arg0TypeInfo.hStruct, mpUniformData, varStr, &extData, (int)sizeof(void*));
			}
		}
		else {
			SC::Int iData[4] = {0,0,0,0};
			SC::Float fData[4] = {0,0,0,0};
			SC::Boolean bData = 0;
			switch (memberType.type) {
			case SC::kBoolean:
				sscanf(valueStr, "<%d>", &bData);
				break;
			case SC::kInt:			
				sscanf(valueStr, "<%d>", &iData[0]);
				break;
			case SC::kInt2:
				sscanf(valueStr, "<%d,%d>", &iData[0], &iData[1]);
				break;
			case SC::kInt3:
				sscanf(valueStr, "<%d,%d,%d>", &iData[0], &iData[1], &iData[2]);
				break;
			case SC::kInt4:
				sscanf(valueStr, "<%d,%d,%d,%d>", &iData[0], &iData[1], &iData[2], &iData[3]);
				break;
			case SC::kFloat:
				sscanf(valueStr, "<%f>", &fData[0]);
				break;
			case SC::kFloat2:
				sscanf(valueStr, "<%f,%f>", &fData[0], &fData[1]);
				break;
			case SC::kFloat3:
				sscanf(valueStr, "<%f,%f,%f>", &fData[0], &fData[1], &fData[2]);
				break;
			case SC::kFloat4:
				sscanf(valueStr, "<%f,%f,%f,%f>", &fData[0], &fData[1], &fData[2], &fData[3]);
				break;
			}

			switch (memberType.type) {
			case SC::kBoolean:
				KSC_SetStructMemberData(arg0TypeInfo.hStruct, mpUniformData, varStr, &bData, (int)sizeof(int));
				break;
			case SC::kInt:
			case SC::kInt2:
			case SC::kInt3:
			case SC::kInt4:
				KSC_SetStructMemberData(arg0TypeInfo.hStruct, mpUniformData, varStr, iData, (int)sizeof(iData));
				break;
			case SC::kFloat:
			case SC::kFloat2:
			case SC::kFloat3:
			case SC::kFloat4:
				KSC_SetStructMemberData(arg0TypeInfo.hStruct, mpUniformData, varStr, fData, (int)sizeof(fData));
				break;
			}
		}
	}

	fclose(f);
	return true;
}

bool KSC_Shader::InitializeUniform(const char* name)
{
	return false;
}


bool KSC_Shader::Validate(FunctionHandle shadeFunc)
{
	return true;
}

void* KSC_Shader::CreateExternalData(const char* typeString, const char* valueString)
{
	return NULL;
}

KSC_Shader::~KSC_Shader()
{

}

void KSC_Shader::Execute(void* inData, void* outData) const
{
	typedef void (*PFN_invoke)(void*, void*, void*);
	PFN_invoke funcPtr = (PFN_invoke)mpFuncPtr;
	funcPtr(mpUniformData, inData, outData);
}

bool KSC_Shader::SetUniformParam(const char* name, void* data, int dataSize)
{
	KSC_TypeInfo uniformArgType = KSC_GetFunctionArgumentType(mShadeFunction, 0);
	if (uniformArgType.hStruct == NULL)
		return false;
	KSC_TypeInfo memberType = KSC_GetStructMemberType(uniformArgType.hStruct, name);
	if (memberType.type == SC::kInvalid)
		return false;
	if (memberType.type == SC::kStructure)
		return false;

	if (memberType.type == SC::kExternType) {
		void* pExtData = CreateExternalData(memberType.typeString, (const char*)data);
		if (pExtData) {
			if (KSC_SetStructMemberData(uniformArgType.hStruct, mpUniformData, name, &pExtData, sizeof(void*))) {
				std::vector<BYTE>& storedData = mModifiedData[name];
				storedData.resize(strlen((const char*)data) + 1);
				memcpy(&storedData[0], data, storedData.size());
				return true;
			}
		}
	}
	else {
		if (dataSize == KSC_GetTypePackedSize(memberType)) {
			if (KSC_SetStructMemberData(uniformArgType.hStruct, mpUniformData, name, data, dataSize)) {
				// Store the successfully modified data
				std::vector<BYTE>& storedData = mModifiedData[name];
				storedData.resize(dataSize);
				memcpy(&storedData[0], data, dataSize);
				return true;
			}
		}
	}
	return false;
}

KSC_ShaderWithTexture::KSC_ShaderWithTexture()
{

}

KSC_ShaderWithTexture::KSC_ShaderWithTexture(const KSC_ShaderWithTexture& ref) :
	KSC_Shader(ref)
{

}

KSC_ShaderWithTexture::~KSC_ShaderWithTexture()
{

}

void* KSC_ShaderWithTexture::CreateExternalData(const char* typeString, const char* valueString)
{
	if (0 == strcmp(typeString, "Texture2D")) {
		return Texture::TextureManager::GetInstance()->CreateBitmapTexture(valueString);
	}
	else if (0 == strcmp(typeString, "Texture3D")) {
		// TODO:
		return NULL;
	}
	else
		return KSC_Shader::CreateExternalData(typeString, valueString);
}

void KSC_ShaderWithTexture::Sample2D(Texture::Tex2D* tex, KVec2* uv, KVec4* outSample)
{
	if (tex) {
		*outSample = tex->SamplePoint(*uv);
	}
	else
		*outSample = KVec4(0,0,0,0);
}