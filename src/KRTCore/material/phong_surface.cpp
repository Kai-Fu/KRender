#include "standard_mtl.h"
#include "../shader/light_scheme.h"
#include "../file_io/file_io_template.h"

void PhongSurface::CalculateShading(const KColor& in_clr, const KVec3& in_dir, const ShadingContext& shadingCtx, KColor& out_clr) const
{
	KColor diffuseCoefficient(1.0f, 1.0f, 1.0f);

	// Calculate the diffuse & specular for each light
	LightIterator li_iter;
	const KVec3& normal = shadingCtx.normal;
	out_clr.Clear();

	float in_side = shadingCtx.normal * shadingCtx.out_vec;
	float out_side = shadingCtx.normal * in_dir;
	if (in_side < 0 || out_side < 0)
		return;

	float diffuse_scale = normal * in_dir;
	float specular_scale = 0;
	if (diffuse_scale < 0)
		diffuse_scale = std::max(0.0f, diffuse_scale);
	else {
		KVec3 reflect = normal * (2*diffuse_scale) - in_dir;
		float rDoti = reflect * shadingCtx.out_vec;
		if (rDoti > 0)
			specular_scale = pow(rDoti, mParam.mPower);
	}

	KColor diffuseClr(in_clr);
	diffuseClr.Modulate(diffuseCoefficient);

	diffuseClr.Scale(diffuse_scale);

	if (mDiffuseMap && shadingCtx.hasUV) {
	
		if (!shadingCtx.is_primary_ray)
			diffuseClr = mDiffuseMap->SampleBilinear(shadingCtx.uv.uv);
		else
			diffuseClr = mDiffuseMap->SampleEWA(shadingCtx.uv.uv, shadingCtx.uv.du, shadingCtx.uv.dv);
	}
	else {
		diffuseClr.r *= mParam.mDiffuse.r;
		diffuseClr.g *= mParam.mDiffuse.g;
		diffuseClr.b *= mParam.mDiffuse.b;
	}

	out_clr.Add(diffuseClr);

	KColor specularClr(in_clr);
	specularClr.Scale(specular_scale);
	specularClr.r *= mParam.mSpecular.r;
	specularClr.g *= mParam.mSpecular.g;
	specularClr.b *= mParam.mSpecular.b;

	out_clr.Add(specularClr);
}

