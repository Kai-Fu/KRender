#pragma once

#include "../camera/camera.h"

#include <common/defines/typedefs.h>
#include "../image/Color.h"
#include "../image/BitmapObject.h"
#include "../api/KRT_API.h"
#include "../image/basic_map.h"
#include <KShaderCompiler/inc/SC_API.h>
#include <hash_map>


struct IntersectInfo
{
	KVec3 pos;
	KVec3 nor;
	UINT32 bbox_node_idx;
	KVec3 face_nor;
	UINT32 tri_id;
	float RefraIdx;
};

struct TangentData {
	KVec3 tangent;
	KVec3 binormal;
};


struct UV_SAMP_INFO
{
	KVec2 uv;
	KVec2 du;
	KVec2 dv;
};


class ISurfaceShader;
class TracingInstance;

struct SurfaceContext
{
	SurfaceContext();
	~SurfaceContext();

	struct TracingData
	{
		TracingInstance* tracing_inst;
		const IntersectContext* hit_ctx;
		const ShadingContext* shading_ctx;

		UINT32 iter_light_li;
		UINT32 iter_light_si;
	};

	void Allocate(const KSC_TypeInfo& kscType);

	TracingData tracerDataLocal;

	void* mpData;

	KVec3* outVec;

	KVec3* normal;
	KVec3* tangent;
	KVec3* binormal;

	KVec2* uv;

	TracingData** tracerData;
};

struct ShadingContext
{
	KVec3 position;
	KVec3 normal;
	TangentData tangent;
	KVec3 out_vec;
	KVec3 face_normal;

	float face_size;
	UINT32 excluding_bbox;
	UINT32 excluding_tri;
	UINT32 hasUV;

	UV_SAMP_INFO uv;

	ISurfaceShader* surface_shader;
	const TracingInstance* tracing_instance;
	bool is_primary_ray;
};


class KKDBBoxScene;
class RenderBuffers;
struct LightIterator;

class TracingInstance
{
public:
	TracingInstance(const KKDBBoxScene* scene, const RenderBuffers* pBuffers);
	~TracingInstance();

	const KKDBBoxScene* GetScenePtr() const;
	KVec2 GetAreaLightSample(UINT32 lightIdx, UINT32 sampleIdx) const;
	void SetCurrentPixel(UINT32 x, UINT32 y);
	void IncBounceDepth();
	void DecBounceDepth();
	UINT32 GetBoundDepth() const;
	KColor GetBackGroundColor(const KVec3& dir) const;

	void CalcuShadingContext(const KRay& hitRay, const IntersectContext& hit_ctx, ShadingContext& out_shading_ctx) const;
	const ISurfaceShader* GetSurfaceShader(const IntersectContext& hit_ctx) const;
	void CalcuHitInfo(const IntersectContext& hit_ctx, IntersectInfo& out_info) const;
	
	bool CastRay(const KRay& ray, IntersectContext& out_ctx) const;
	bool IsPointOccluded(const KRay& ray, float len) const;

	void ConvertToSurfaceContext(const IntersectContext& hitCtx, const ShadingContext& shadingCtx, SurfaceContext& surfaceCtx);
	SurfaceContext& GetCurrentSurfaceCtxStorage();
public:
	KCamera::EvalContext mCameraContext;

private:
	const KKDBBoxScene* mpScene;
	const RenderBuffers* mpRenderBuffers;
	std::vector<SurfaceContext> mSurfaceContexts;
	UINT32 mBounceDepth;
	UINT32 mCurPixel_X;
	UINT32 mCurPixel_Y;
	bool mIsPixelSampling;
};

// structure passed to surface shader to light iteration
struct LightIterator
{
	KVec3 direction;
	KColor intensity;
};

class RenderBuffers
{
private:
	std::auto_ptr<BitmapObject>		output_image;
	// sampled count per-pixel
	std::vector<UINT32>		sampled_count_pp;
	// random seed per-pixel
	std::vector<UINT32>		random_seed_pp;
	// random float value(between 0 and 1) sequence
	std::vector<float>		random_sequence;
public:
	void SetImageSize(UINT32 w, UINT32 h, KRT_ImageFormat pixelFormat, void* pUserBuf);
	void AddSamples(UINT32 w, UINT32 h, UINT32 sampleCnt, const KColor& avgClr, float alpha);
	UINT32 GetSampledCount(UINT32 x, UINT32 y) const;
	void IncreaseSampledCount(UINT32 x, UINT32 y, UINT32 sampleCnt);
	UINT32 GetRandomSeed(UINT32 x, UINT32 y) const;
	float GetPixelRandom(UINT32 x, UINT32 y, UINT32 offset) const;
	float GetPixelRandom(UINT32 x, UINT32 y, UINT32 offset, float min, float max) const;

	KVec2 RS_Image(UINT32 x, UINT32 y) const;
	KVec2 RS_DOF(UINT32 x, UINT32 y, const KVec2& apertureSize) const;
	KVec2 RS_AreaLight(UINT32 x, UINT32 y, UINT32 lightIdx, UINT32 sampleIdx) const;
	float RS_MotionBlur(UINT32 x, UINT32 y) const;
	const BitmapObject* GetOutputImagePtr() const;

};

struct RenderParam {
	bool is_refining;
	bool want_motion_blur;
	bool want_depth_of_field;
	bool want_global_illumination;
	bool want_edge_sampling;
	UINT32 sample_cnt_eval;
	UINT32 sample_cnt_more;
	UINT32 sample_cnt_edge;

	UINT32 image_width;
	UINT32 image_height;
	void* user_buffer;
	KRT_ImageFormat pixel_format; 
};



// Shader based on KShaderCompiler
class KSC_Shader
{
public:
	KSC_Shader();
	KSC_Shader(const KSC_Shader& ref);
	~KSC_Shader();

	bool LoadTemplate(const char* templateFile);
	void Execute(void* inData, void* outData) const;
	bool SetUniformParam(const char* name, void* data, int dataSize);

	virtual bool HandleModule(ModuleHandle kscModule) = 0;
	virtual bool InitializeUniform(const char* name);
	virtual bool Validate(FunctionHandle shadeFunc);
	virtual void* CreateExternalData(const char* typeString, const char* valueString);

protected:
	void* mpUniformData;
	KSC_TypeInfo mUnifomArgType;

	void* mpFuncPtr;
	FunctionHandle mShadeFunction;

	std::hash_map<std::string, std::vector<BYTE> > mModifiedData;

private:
	static std::hash_map<std::string, FunctionHandle> sLoadedShadeFunctions;
};

class KSC_ShaderWithTexture : public KSC_Shader
{
public:
	KSC_ShaderWithTexture();
	KSC_ShaderWithTexture(const KSC_ShaderWithTexture& ref);
	~KSC_ShaderWithTexture();

	virtual void* CreateExternalData(const char* typeString, const char* valueString);

	static void Sample2D(Texture::Tex2D* tex, KVec2* uv, KVec4* outSample);
};