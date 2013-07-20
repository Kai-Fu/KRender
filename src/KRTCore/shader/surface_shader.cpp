#include "surface_shader.h"
#include "../entry/TracingThread.h"
#include "../util/HelperFunc.h"
#include "light_scheme.h"
#include "../intersection/intersect_ray_bbox.h"
#include "../shader/environment_shader.h"


bool CalcuShadingByRay(TracingInstance* pLocalData, const KRay& ray, KColor& out_clr, IntersectContext* out_ctx/* = NULL*/)
{
	const LightScheme* pLightScheme = LightScheme::GetInstance();
	const KAccelStruct_BVH* pScene = pLocalData->GetScenePtr();
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
		// This ray hits something, shade the ray sample by surface shader of the hit object.
		//
		if (out_ctx)
			*out_ctx = hit_ctx;
		
		// Calculate the data in shading context
		ShadingContext shadingCtx;
		pLocalData->CalcuShadingContext(ray, hit_ctx, shadingCtx);
		if (shadingCtx.surface_shader) {

			if (shadingCtx.surface_shader->mNormalMap && shadingCtx.hasUV) {
				KVec4 samp_res;
				samp_res = shadingCtx.surface_shader->mNormalMap->SampleBilinear(shadingCtx.uv.uv);
				KVec3 normal = shadingCtx.tangent.tangent * (samp_res[0] *2.0f - 1.0f);
				normal += (shadingCtx.tangent.binormal * (samp_res[1] * 2.0f - 1.0f));
				normal += (shadingCtx.normal * (samp_res[2] * 2.0f - 1.0f));
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
		else {
			// No surface shader? just output its normal
			out_clr.r = shadingCtx.normal[0] * 0.5f + 0.5f;
			out_clr.g = shadingCtx.normal[1] * 0.5f + 0.5f;
			out_clr.b = shadingCtx.normal[2] * 0.5f + 0.5f;
		}
	}
	else {
		// This ray hits nothing, so it should evaluate the environment shader to determine current color of the ray sample.
		//
		const KEnvShader* pEnvShader = KEnvShader::GetEnvShader();
		if (pEnvShader) {
			pEnvShader->Sample(ray.GetOrg(), ray.GetDir(), out_clr);
		}
	}

	pLocalData->DecBounceDepth();
	return res;
}

bool CalcSecondaryRay(TracingInstance* pLocalData, const KVec3& org, UINT32 excludingBBox, UINT32 excludingTri, const KVec3& ray_dir, KColor& out_clr)
{
	KRay secondaryRay;
	secondaryRay.Init(org, ray_dir, NULL);
	secondaryRay.mExcludeBBoxNode = excludingBBox;
	secondaryRay.mExcludeTriID = excludingTri;
	return CalcuShadingByRay(pLocalData, secondaryRay, out_clr, NULL);
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