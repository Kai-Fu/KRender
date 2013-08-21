#pragma once

#include "../base/geometry.h"
#include "shader_api.h"

#include "../image/color.h"
#include <assert.h>
#include <vector>
#include <set>

#define SAMPLE_CTX_SIZE 100
#define POINT_LIGHT_TYPE "basic_point_light"
#define RECT_LIGHT_TYPE "basic_rectangle_light"
#define GENERIC_LIGHT_TYPE "generic_light"

struct ShadingContext;
class LightScheme;


class ILightObject
{
	friend class LightScheme;
public:
	virtual ~ILightObject() {}

	virtual bool IsAreaLight() const = 0;
	virtual bool EvaluateLighting(
		const KVec2& samplePos, const KVec3& shading_point,
		KVec3& outLightPos, LightIterator& outLightIter) const = 0;


	virtual void SetLightSpaceMatrix(const KMatrix4& mat) = 0;
	virtual void SetParam(const char* paramName, void* pData) = 0;

	virtual const char* GetType() const = 0;

	virtual bool Save(FILE* pFile) = 0;
	virtual bool Load(FILE* pFile) = 0;

	static float RandomFloat();
	static void ConfigAreaLightSampCnt(UINT32 cntSqrt);
protected:


};

class PointLightBase : public ILightObject
{
public:
	PointLightBase();
	virtual ~PointLightBase();
	
	virtual void SetLightSpaceMatrix(const KMatrix4& mat);
	void SetPos(const KVec3& pos);
	KVec3 GetPos() const;
	void SetIntensity(const KColor& clr);
	virtual void SetParam(const char* paramName, void* pData);

	virtual bool IsAreaLight() const {return false;}
	virtual bool EvaluateLighting(
		const KVec2& samplePos, const KVec3& shading_point,
		KVec3& outLightPos, LightIterator& outLightIter) const;


	virtual const char* GetType() const {return POINT_LIGHT_TYPE;}

	virtual bool Save(FILE* pFile);
	virtual bool Load(FILE* pFile);
protected:
	KVec3 mPos;
	KMatrix4 mLightMat;
	KColor mIntensity;

};

class RectLightBase : public ILightObject
{
public:
	RectLightBase(float w, float h);
	virtual ~RectLightBase();

	void SetSize(float w, float h);
	virtual void SetLightSpaceMatrix(const KMatrix4& mat);

	virtual bool IsAreaLight() const {return true;}
	virtual bool EvaluateLighting(
		const KVec2& samplePos, const KVec3& shading_point,
		KVec3& outLightPos, LightIterator& outLightIter) const;;


	virtual void SetParam(const char* paramName, void* pData);

	virtual const char* GetType() const {return RECT_LIGHT_TYPE;}

	virtual bool Save(FILE* pFile);
	virtual bool Load(FILE* pFile);
protected:

	struct PARAM {
		float mSizeX;
		float mSizeY;
		KColor mIntensity;

		KMatrix4 mLightMat; // the rectangle light is on the XOY plane of this axis
		KVec3 mCornerPos[4];
		KVec3 mEdgeDir[2];
	};
	PARAM mParams;
};

class ISurfaceShader;
// the manager of light source objects
class LightScheme
{
public:
	LightScheme();
	~LightScheme();

	void ClearLightSource();
	UINT32 GetLightCount() const;
	UINT32 GetPointLightCount() const;
	ILightObject* CreateLightSource(const char* type);
	bool RemoveLightSource(ILightObject* pLight);
	ILightObject* GetLightPtr(UINT32 lightIdx);
	const ILightObject* GetLightPtr(UINT32 lightIdx) const;
	bool GetLightIter(TracingInstance* pLocalData, const KVec2& samplePos, UINT32 lightIdx, const ShadingContext* shadingCtx,  const IntersectContext* hit_ctx, LightIterator& out_iter) const;


	void Shade(TracingInstance* pLocalData, 
		const ShadingContext& shadingCtx, 
		const IntersectContext& hit_ctx, 
		KColor& out_color) const;

	bool Save(FILE* pFile);
	bool Load(FILE* pFile);

	static LightScheme* GetInstance();
	static void Initialize();
	static void Shutdown();

private:
	void AdjustHitPos(TracingInstance* pLocalData, const IntersectContext& hit_ctx, const ShadingContext& shadingCtx, KVec3d& in_out_pos) const;

protected:
	std::vector<ILightObject*> mpLights;

	static LightScheme* s_pInstance;
};


