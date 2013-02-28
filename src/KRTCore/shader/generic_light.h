#pragma once
#include "light_scheme.h"

class GenericLight : public ILightObject
{
public:
	GenericLight();
	virtual ~GenericLight();

	virtual bool EvaluateLighting(
		UINT32& sampIter, const KVec3& shading_point, float pixelMagic, 
		KVec3& outLightPos, LightIterator& outLightIter) const;

	virtual void SetParam(const char* paramName, void* pData);

	virtual const char* GetType() const {return GENERIC_LIGHT_TYPE;}

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