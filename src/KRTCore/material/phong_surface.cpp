#include "standard_mtl.h"
#include "../shader/light_scheme.h"
#include "../file_io/file_io_template.h"

void PhongSurface::Shade(const SurfaceContext& shadingCtx, KColor& out_clr) const
{
	KColor diffuseCoefficient(1.0f, 1.0f, 1.0f);

	// Calculate the diffuse & specular for each light
	LightIterator li_iter;
	const KVec3& normal = *shadingCtx.normal;
	out_clr.Clear();
	/*
	float in_side = *shadingCtx.normal * *shadingCtx.outVec;
	float out_side = *shadingCtx.normal * *shadingCtx.inVec;
	if (in_side < 0 || out_side < 0)
		return;

	float diffuse_scale = normal * *shadingCtx.inVec;
	float specular_scale = 0;
	if (diffuse_scale < 0)
		diffuse_scale = std::max(0.0f, diffuse_scale);
	else {
		KVec3 reflect = normal * (2*diffuse_scale) - *shadingCtx.inVec;
		float rDoti = reflect * *shadingCtx.outVec;
		if (rDoti > 0)
			specular_scale = pow(rDoti, mParam.mPower);
	}

	KColor diffuseClr(*shadingCtx.inLight);
	diffuseClr.Modulate(diffuseCoefficient);

	diffuseClr.Scale(diffuse_scale);*/

	/*if (mDiffuseMap && shadingCtx.hasUV) {
	
		if (!shadingCtx.is_primary_ray)
			diffuseClr = mDiffuseMap->SampleBilinear(shadingCtx.uv.uv);
		else
			diffuseClr = mDiffuseMap->SampleEWA(shadingCtx.uv.uv, shadingCtx.uv.du, shadingCtx.uv.dv);
	}
	else*/ /*{
		diffuseClr.r *= mParam.mDiffuse.r;
		diffuseClr.g *= mParam.mDiffuse.g;
		diffuseClr.b *= mParam.mDiffuse.b;
	}

	out_clr.Add(diffuseClr);

	KColor specularClr(*shadingCtx.inLight);
	specularClr.Scale(specular_scale);
	specularClr.r *= mParam.mSpecular.r;
	specularClr.g *= mParam.mSpecular.g;
	specularClr.b *= mParam.mSpecular.b;

	out_clr.Add(specularClr);*/
}

