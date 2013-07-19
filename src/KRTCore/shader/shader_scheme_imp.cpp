#include "light_scheme.h"
#include "surface_shader.h"

#include "../entry/TracingThread.h"
#include "../file_io/file_io_template.h"
#include "../intersection/intersect_ray_bbox.h"

extern UINT32 AREA_LIGHT_SAMP_CNT;

void LightScheme::AdjustHitPos(TracingInstance* pLocalData, const IntersectContext& hit_ctx, const ShadingContext& shadingCtx, KVec3& in_out_pos) const
{
	const KKDBBoxScene* pScene = pLocalData->GetScenePtr();
	const KScene* pKDScene = pScene->GetSource()->GetNodeKDScene(hit_ctx.bbox_node_idx);
	const KAccelTriangle* pTri = pScene->GetAccelTriData(hit_ctx.bbox_node_idx, hit_ctx.tri_id);
	UINT32 mesh_idx = pTri->GetMeshIdx();
	UINT32 tri_idx = pTri->mTriIdx;
	const KTriMesh* pMesh = pKDScene->GetMesh(mesh_idx);
	const UINT32* nor_idx = pMesh->mFaces[tri_idx].pn_idx;
	
	KVec3 temp_vec = 
		pMesh->mVertPN[nor_idx[0]].nor +
		pMesh->mVertPN[nor_idx[1]].nor +
		pMesh->mVertPN[nor_idx[2]].nor;
	float offset_scale = 3.0f - nvmath::length(temp_vec);

	float u = hit_ctx.u;
	float v = hit_ctx.v;
	float w = hit_ctx.w;
	offset_scale *= (1.0f - u*u - v*v - w*w); 
	offset_scale *= (shadingCtx.face_size * 10.0f);
	if (offset_scale > shadingCtx.face_size) offset_scale = shadingCtx.face_size;
	in_out_pos += shadingCtx.normal * offset_scale;

}

bool LightScheme::GetLightIter(TracingInstance* pLocalData, const KVec2& samplePos, UINT32 lightIdx, const ShadingContext* shadingCtx,  const IntersectContext* hit_ctx, LightIterator& out_iter) const
{
	const ILightObject* pLightObj = GetLightPtr(lightIdx);
		
	KVec3 lightPos;
	KColor litClr;
	if (pLightObj->EvaluateLighting(samplePos, shadingCtx->position, lightPos, out_iter)) {

		bool isOccluded = false;
		KColor transmission(out_iter.intensity);
		{
			KVec3 adjustedPos = shadingCtx->position;
			KVec3 lightDir = lightPos - adjustedPos;
			float l_n = lightDir * shadingCtx->normal;
			float l_fn = lightDir * shadingCtx->face_normal;
			if (l_n > 0) {

				if (l_fn < 0) 
					AdjustHitPos(pLocalData, *hit_ctx, *shadingCtx, adjustedPos);

				KVec3 temp_light_pos(lightPos);
				lightDir = adjustedPos - temp_light_pos;
					
				// Clamp the shadow ray if the light source is out of the scene's bounding box.
				// Doing so can improve the floating point precision.
				const KBBox& sceneBBox = pLocalData->GetScenePtr()->GetSceneBBox();
				bool needClampRay = !sceneBBox.IsInside(temp_light_pos);
				if (needClampRay) {
					KRay ray;
					ray.Init(temp_light_pos, lightDir, NULL);
					float t0, t1;
					if (IntersectBBox(ray, sceneBBox, t0, t1)) {
						temp_light_pos = ray.GetOrg() + ray.GetDir() * t0;
					}

				}
					
				KRay ray;
				lightDir = adjustedPos - temp_light_pos;
				ray.Init(temp_light_pos, lightDir, NULL);
				ray.mExcludeBBoxNode = INVALID_INDEX;
				ray.mExcludeTriID = INVALID_INDEX;

				float lum = 1.0f;
				while (lum > 0) {
					IntersectContext test_ctx;
					test_ctx.t = 1.0f;
					isOccluded = pLocalData->CastRay(ray, test_ctx);
					if (isOccluded) {
						if (test_ctx.bbox_node_idx == hit_ctx->bbox_node_idx && test_ctx.tri_id == hit_ctx->tri_id)
							break;

						// Calculate how much light can pass through the hit surface
						KColor temp_trans;
						ShadingContext shading_context;
						pLocalData->CalcuShadingContext(ray, test_ctx, shading_context);
						TransContext& transCtx = pLocalData->GetCurrentTransCtxStorage();
						pLocalData->ConvertToTransContext(test_ctx, shading_context, transCtx);
						shading_context.surface_shader->ShaderTransmission(transCtx, temp_trans);
						transmission.Modulate(temp_trans);

						lum = transmission.Luminance();
						if (lum > 0.001f) {
							KVec3 go_dis = lightDir*test_ctx.t;
							temp_light_pos += (go_dis * 1.00001f);
							lightDir -= (go_dis * 0.9999f);
							ray.Init(temp_light_pos, lightDir, NULL);
							ray.mExcludeBBoxNode = test_ctx.bbox_node_idx;
							ray.mExcludeTriID = test_ctx.tri_id; 

						}
						else {
							transmission.Clear();
							break;
						}
					}
					else 
						break;

				}
			}
			else
				transmission.Clear();

		}

		if (transmission.Luminance() > 0) {
			out_iter.intensity = transmission;
			return true;
		}
	}

	return false;
}

void LightScheme::Shade(TracingInstance* pLocalData, 
						const ShadingContext& shadingCtx, 
						const IntersectContext& hit_ctx, 
						KColor& out_color) const
{
	const KKDBBoxScene* pScene = pLocalData->GetScenePtr();
	const KScene* pKDScene = pScene->GetSource()->GetNodeKDScene(hit_ctx.bbox_node_idx);

	SurfaceContext& surfaceCtx = pLocalData->GetCurrentSurfaceCtxStorage();
	pLocalData->ConvertToSurfaceContext(hit_ctx, shadingCtx, surfaceCtx);

	KColor tempClr;
	shadingCtx.surface_shader->Shade(surfaceCtx, tempClr);
	out_color = tempClr;
}


bool PointLightBase::EvaluateLighting(
		const KVec2& samplePos, const KVec3& shading_point,
		KVec3& outLightPos, LightIterator& outLightIter) const
{
	outLightPos = mPos;

	KVec3 temp_vec = mPos - shading_point;
	float rcpLenSqr = 1.0f / nvmath::lengthSquared(temp_vec);
	outLightIter.direction = temp_vec * sqrtf(rcpLenSqr);
	outLightIter.intensity = mIntensity;

	// Attenuate?
	//outLightIter.intensity.Scale(rcpLenSqr);

	return true;
}


bool RectLightBase::EvaluateLighting(
		const KVec2& samplePos, const KVec3& shading_point,
		KVec3& outLightPos, LightIterator& outLightIter) const
{
	KVec3 tempPos;
	tempPos[0] = mParams.mSizeX * (samplePos[0] - 0.5f);
	tempPos[1] = mParams.mSizeY * (samplePos[1] - 0.5f);
	tempPos[2] = 0;

	Vec3TransformCoord(outLightPos, tempPos, mParams.mLightMat);

	// evaluate the lighting
	KVec3 temp_vec = outLightPos - shading_point;
	float rcpLenSqr = 1.0f / (temp_vec[0]*temp_vec[0] + temp_vec[1]*temp_vec[1] + temp_vec[2]*temp_vec[2]);
	outLightIter.direction = temp_vec * sqrtf(rcpLenSqr);

	outLightIter.intensity.r = mParams.mIntensity.r;
	outLightIter.intensity.g = mParams.mIntensity.g;
	outLightIter.intensity.b = mParams.mIntensity.b;

	return false;
}


