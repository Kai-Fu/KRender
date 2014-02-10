#include "light_scheme.h"
#include "surface_shader.h"


extern UINT32 AREA_LIGHT_SAMP_CNT;


LightScheme* LightScheme::s_pInstance = NULL;
LightScheme* LightScheme::GetInstance()
{
	assert(s_pInstance);
	return s_pInstance;
}

void LightScheme::Initialize()
{
	s_pInstance = new LightScheme();
}

void LightScheme::Shutdown()
{
	delete s_pInstance;
	s_pInstance = NULL;
}

LightScheme::LightScheme()
{
}

ILightObject* LightScheme::CreateLightSource(const char* type)
{
	ILightObject* pLight = NULL;
	if (0 == strcmp(type, POINT_LIGHT_TYPE)) {
		pLight = new PointLightBase();
	}
	else if (0 == strcmp(type, RECT_LIGHT_TYPE)) {
		pLight = new RectLightBase(30.0f, 30.0f);
	}

	if (pLight)
		mpLights.push_back(pLight);
	return pLight;
}

bool LightScheme::RemoveLightSource(ILightObject* pLight)
{
	std::vector<ILightObject*>::iterator it = std::find(mpLights.begin(), mpLights.end(), pLight);
	if (it != mpLights.end()) {
		mpLights.erase(it);
		delete pLight;
		return true;
	}
	else
		return false;
}

LightScheme::~LightScheme()
{
	ClearLightSource();

}

UINT32 LightScheme::GetLightCount() const
{
	return (UINT32)mpLights.size();
}

UINT32 LightScheme::GetPointLightCount() const
{
	return (UINT32)mpLights.size();
}

void LightScheme::ClearLightSource()
{
	for (std::vector<ILightObject*>::iterator it = mpLights.begin(); it !=  mpLights.end(); ++it) {
		delete *it;
	}
	mpLights.clear();

}

ILightObject* LightScheme::GetLightPtr(UINT32 lightIdx)
{
	ILightObject* ret = mpLights[lightIdx];
	return ret;
}

const ILightObject* LightScheme::GetLightPtr(UINT32 lightIdx) const
{
	const ILightObject* ret = mpLights[lightIdx];
	return ret;
}

float ILightObject::RandomFloat()
{
	return rand() / (float)RAND_MAX;
}

PointLightBase::PointLightBase()
{
	mPos[0] = mPos[1] = mPos[2] = 0.0f;
	mIntensity.r = mIntensity.g = mIntensity.b = 1.0f;
	mLightMat = nvmath::cIdentity44f;
}

PointLightBase::~PointLightBase()
{

}

void PointLightBase::SetLightSpaceMatrix(const KMatrix4& mat)
{
	mLightMat = mat;
	mPos[0] = mLightMat[3][0];
	mPos[1] = mLightMat[3][1];
	mPos[2] = mLightMat[3][2];
}

void PointLightBase::SetPos(const KVec3& pos)
{
	mPos = pos;
	mLightMat[3][0] = mPos[0];
	mLightMat[3][1] = mPos[1];
	mLightMat[3][2] = mPos[2];
}

KVec3 PointLightBase::GetPos() const
{
	return mPos;
}

void PointLightBase::SetIntensity(const KColor& clr)
{
	mIntensity = clr;
}

RectLightBase::RectLightBase(float w, float h)
{
	mParams.mLightMat = nvmath::cIdentity44f;
	SetSize(w, h);
}

RectLightBase::~RectLightBase()
{

}

void RectLightBase::SetSize(float w, float h)
{
	mParams.mSizeX = w;
	mParams.mSizeY = h;
	KVec3 pos;
	pos[2] = 0.0f;

	pos[0] = w * 0.5f; pos[1] = h * 0.5f;
	Vec3TransformCoord(mParams.mCornerPos[0], pos, mParams.mLightMat);
	pos[0] = w * 0.5f; pos[1] = h * -0.5f;
	Vec3TransformCoord(mParams.mCornerPos[1], pos, mParams.mLightMat);

	pos[0] = w * -0.5f; pos[1] = h * 0.5f;
	Vec3TransformCoord(mParams.mCornerPos[2], pos, mParams.mLightMat);
	pos[0] = w * -0.5f; pos[1] = h * -0.5f;
	Vec3TransformCoord(mParams.mCornerPos[3], pos, mParams.mLightMat);

	mParams.mEdgeDir[0] = mParams.mCornerPos[0] - mParams.mCornerPos[2];
	mParams.mEdgeDir[1] = mParams.mCornerPos[0] - mParams.mCornerPos[1];

}

void RectLightBase::SetLightSpaceMatrix(const KMatrix4& mat)
{
	mParams.mLightMat = mat;
	SetSize(mParams.mSizeX, mParams.mSizeY);
}

void PointLightBase::SetParam(const char* paramName, void* pData)
{
	if (0 == strcmp(paramName, "intensity"))
		memcpy(&mIntensity, pData, sizeof(KColor));

}

void RectLightBase::SetParam(const char* paramName, void* pData)
{
	if (0 == strcmp(paramName, "intensity"))
		memcpy(&mParams.mIntensity, pData, sizeof(KColor));
	else if (0 == strcmp(paramName, "size_x"))
		memcpy(&mParams.mSizeX, pData, sizeof(float));
	else if (0 == strcmp(paramName, "size_y"))
		memcpy(&mParams.mSizeY, pData, sizeof(float));
}
