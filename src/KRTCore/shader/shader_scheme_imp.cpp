#include "light_scheme.h"
#include "surface_shader.h"

#include "../entry/tracing_thread.h"
#include "../intersection/intersect_ray_bbox.h"

extern UINT32 AREA_LIGHT_SAMP_CNT;

void LightScheme::AdjustHitPos(TracingInstance* pLocalData, const IntersectContext& hit_ctx, const ShadingContext& shadingCtx, KVec3d& in_out_pos) const
{
	const KAccelStruct_BVH* pScene = pLocalData->GetScenePtr();
	const KScene* pKDScene = pScene->GetSource()->GetNodeKDScene(hit_ctx.bbox_node_idx);
	const KTriDesc* pTri = pScene->GetAccelTriData(hit_ctx.bbox_node_idx, hit_ctx.tri_id);
	UINT32 mesh_idx = pTri->GetMeshIdx();
	UINT32 tri_idx = pTri->mTriIdx;
	const KTriMesh* pMesh = pKDScene->GetMesh(mesh_idx);
	const UINT32* nor_idx = pMesh->mFaces[tri_idx].pn_idx;
	
	KVec3 temp_vec = 
		pMesh->ComputeVertNor(nor_idx[0], pLocalData->mCameraContext.inMotionTime) +
		pMesh->ComputeVertNor(nor_idx[1], pLocalData->mCameraContext.inMotionTime) +
		pMesh->ComputeVertNor(nor_idx[2], pLocalData->mCameraContext.inMotionTime);
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
		
	KVec3 lightPos_f;
	KVec3d lightPos;
	KColor litClr;
	if (pLightObj->EvaluateLighting(samplePos, shadingCtx->position, lightPos_f, out_iter)) {

		lightPos = ToVec3d(lightPos_f);
		bool isOccluded = false;
		KColor transmission(out_iter.intensity);
		{
			KVec3d adjustedPos = ToVec3d(shadingCtx->position);
			KVec3d lightDir = lightPos - adjustedPos;
			float l_n = ToVec3f(lightDir) * shadingCtx->normal;
			float l_fn = ToVec3f(lightDir) * shadingCtx->face_normal;

			if (l_n > 0 && l_fn < 0) 
				AdjustHitPos(pLocalData, *hit_ctx, *shadingCtx, adjustedPos);

			KVec3d temp_light_pos(lightPos);
			lightDir = adjustedPos - temp_light_pos;
					
			KRay ray;
			lightDir = temp_light_pos - adjustedPos;
			ray.Init(adjustedPos, lightDir, NULL);
			ray.mExcludeBBoxNode = hit_ctx->bbox_node_idx;
			ray.mExcludeTriID = hit_ctx->tri_id;

			KColor trans_coefficent;
			pLocalData->ComputeLightTransimission(ray, 1.0f, trans_coefficent);
			transmission.Modulate(trans_coefficent);

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
	const KAccelStruct_BVH* pScene = pLocalData->GetScenePtr();
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


