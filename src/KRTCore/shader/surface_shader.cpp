#include "surface_shader.h"
#include "../entry/TracingThread.h"
#include "../util/HelperFunc.h"
#include "light_scheme.h"
#include "../material/standard_mtl.h"
#include "../intersection/intersect_ray_bbox.h"


bool CalcuShadingByRay(TracingInstance* pLocalData, const KRay& ray, KColor& out_clr, IntersectContext* out_ctx/* = NULL*/)
{
	const LightScheme* pLightScheme = LightScheme::GetInstance();
	const KKDBBoxScene* pScene = pLocalData->GetScenePtr();
	UINT32 rayBounceDepth = pLocalData->GetBoundDepth();
	// Check the maximum bounce depth
	if (rayBounceDepth >= MAX_REFLECTION_BOUNCE) {
		out_clr = pLocalData->GetBackGroundColor(ray.GetDir());
		return false;
	}
	pLocalData->IncBounceDepth();

	IntersectContext hit_ctx;
	if (out_ctx) {
		hit_ctx.bbox_node_idx = out_ctx->bbox_node_idx;
		hit_ctx.kd_leaf_idx = out_ctx->kd_leaf_idx;
	}

	bool res = false;

	bool isHit = false;
	KColor irradiance;
	
	if ((pLocalData->CastRay(ray, hit_ctx))) 
			isHit = true;
	
	out_clr.Clear();

	if (isHit) {

		if (out_ctx)
			*out_ctx = hit_ctx;
		
		// Calculate the data in shading context
		ShadingContext shadingCtx;
		pLocalData->CalcuShadingContext(ray, hit_ctx, shadingCtx);
		if (shadingCtx.surface_shader) {

			if (shadingCtx.surface_shader->mNormalMap && shadingCtx.hasUV) {
				KColor samp_res;
				samp_res = shadingCtx.surface_shader->mNormalMap->SampleBilinear(shadingCtx.uv.uv);
				KVec3 normal = shadingCtx.tangent.tangent * (samp_res.r *2.0f - 1.0f);
				normal += (shadingCtx.tangent.binormal * (samp_res.g * 2.0f - 1.0f));
				normal += (shadingCtx.normal * (samp_res.b * 2.0f - 1.0f));
				nvmath::normalize(normal);

				float irrad_scale = 1.0f;
				irrad_scale = normal * shadingCtx.normal;
				irradiance.Scale(irrad_scale);

				shadingCtx.normal = normal;
			}

		
			out_clr = irradiance;
			pLightScheme->Shade(pLocalData, shadingCtx, hit_ctx, out_clr);
			res = true;
		}
	}
	pLocalData->DecBounceDepth();
	return res;
}

bool CalcReflectedRay(TracingInstance* pLocalData, const ShadingContext& shadingCtx, KColor& reflectColor)
{
	KRay reflectRay;
	reflectRay.InitReflectionRay(shadingCtx, shadingCtx.out_vec, NULL);
	return CalcuShadingByRay(pLocalData, reflectRay, reflectColor, NULL);
}


bool CalcRefractedRay(TracingInstance* pLocalData, const ShadingContext& shadingCtx, float refractRatio, KColor& refractColor)
{
	KRay transRay;
	// TODO: implement the refraction computation
	transRay.InitTranslucentRay(shadingCtx, NULL);
	return CalcuShadingByRay(pLocalData, transRay, refractColor, NULL);
}