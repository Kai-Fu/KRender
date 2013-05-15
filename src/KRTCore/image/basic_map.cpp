#include "basic_map.h"
#include <string>
#include <common/math/nvmath.h>
#include "txfilt.h"

namespace Texture {


static inline int FloatToInt(float x)
{
	unsigned e = (0x7F + 31) - ((* (unsigned*) &x & 0x7F800000) >> 23);
	unsigned m = 0x80000000 | (* (unsigned*) &x << 8);
	return int((m >> e) & -(e < 32));
}

class BitmapData : public TextureMap
{
public:
	float mWidthFactor;
	float mHeightFactor;
	UINT32 mActualWidth;
	UINT32 mActualHeight;
	int mVirtualSize;
	int mLevelCnt;
	int mBitmapObjCnt;
	bool mIsFixPointPixel;
	UINT32 mChannelCnt;
	std::vector<BitmapObject*> mMipmap;

public:
	BitmapData(const char* filename, bool genMipmap)
	{
		BitmapObject* pTop = new BitmapObject();
	
		if (pTop->Load(filename, genMipmap ? &mMipmap : NULL)) {
			mMipmap.insert(mMipmap.begin(), pTop);
			mActualWidth = pTop->mWidth;
			mActualHeight = pTop->mHeight;
			UINT32 largerSize = std::max(pTop->mWidth, pTop->mHeight);
			mVirtualSize = nvmath::powerOfTwoAbove(largerSize);
			mWidthFactor = (float)pTop->mWidth / (float)mVirtualSize;
			mHeightFactor = (float)pTop->mHeight / (float)mVirtualSize;
			mBitmapObjCnt = (int)mMipmap.size();

			if (genMipmap && mVirtualSize != largerSize)
				mMipmap.push_back(mMipmap.back());
			mLevelCnt = (int)mMipmap.size();
		}
	
		if (pTop->mFormat == BitmapObject::eRGB8 || pTop->mFormat == BitmapObject::eRGBA8)
			mIsFixPointPixel = true;
		else
			mIsFixPointPixel = false;

		switch (pTop->mFormat) {
		case BitmapObject::eRGB8:
		case BitmapObject::eRGB32F:
			mChannelCnt = 3;
			break;
		case BitmapObject::eRGBA8:
		case BitmapObject::eRGBA32F:
			mChannelCnt = 3;
			break;
		case BitmapObject::eR32F:
			mChannelCnt = 1;
			break;
		}
	}

	~BitmapData()
	{
		if (!mMipmap.empty()) {
			for (int i = 0; i < mBitmapObjCnt; ++i)
				delete mMipmap[i];
			mMipmap.clear();
		}
	}

	bool IsOk() const
	{
		return !mMipmap.empty();
	}

	int size() const
	{
		return mVirtualSize;
	}

	int levels() const
	{
		return mLevelCnt;
	}

	KVec4 texel(int u, int v, int lod) const
	{
		BitmapObject* pLevel = mMipmap[mLevelCnt - lod - 1]; 
		UINT32 cu = FloatToInt(u * mWidthFactor);
		UINT32 cv = FloatToInt(v * mHeightFactor);
		if (cu >= pLevel->mWidth) cu = pLevel->mWidth - 1;
		if (cv >= pLevel->mHeight) cv = pLevel->mHeight - 1;

		BYTE* pPixel = (BYTE*)pLevel->GetPixelPtr(cu, cv);
		KVec4 ret(0,0,0,1);
		if (mIsFixPointPixel) {

			UINT32 dataSrc = *(UINT32*)pPixel;
			float* dataDst = (float*)&ret;
			for (UINT32 i = 0; i < mChannelCnt; ++i) {
				dataDst[i] = (float(dataSrc & 0x0000ff) / 255.0f);
				dataSrc >>= 8;
			}
		}
		else {

			float* dataSrc = (float*)pPixel;
			float* dataDst = (float*)&ret;
			memcpy(dataDst, dataSrc, sizeof(float)*mChannelCnt);
		}

		return ret;
	}

	KVec4 texel(int u, int v) const
	{
		return texel(u, v, mLevelCnt - 1);
	}

};

Mipmap2D::Mipmap2D()
{
	mpData = NULL;
}

Mipmap2D::~Mipmap2D()
{
	if (mpData)
		delete (BitmapData*)mpData;
	mpData = NULL;
}

bool Mipmap2D::SetSourceFile(const char* filename)
{
	if (mpData)
		delete (BitmapData*)mpData;

	mpData = new BitmapData(filename, true);
	if (!((BitmapData*)mpData)->IsOk()) {
		delete (BitmapData*)mpData;
		mpData = NULL;
		return false;
	}
	else
		return true;
}

KVec4 Mipmap2D::SamplePoint(const KVec2& uv) const
{
	assert(mpData);
	TextureFilter filter;
	return filter.SampleBilinear((BitmapData*)mpData, uv);
}

KVec4 Mipmap2D::SampleBilinear(const KVec2& uv) const
{
	assert(mpData);
	TextureFilter filter;
	return filter.SampleBilinear((BitmapData*)mpData, uv);
}

KVec4 Mipmap2D::SampleTrilinear(const KVec2& uv, const KVec2& du, const KVec2& dv) const
{
	assert(mpData);
	TextureFilter filter;
	return filter.SampleTrilinear((BitmapData*)mpData, uv, du, dv);
}

KVec4 Mipmap2D::SampleEWA(const KVec2& uv, const KVec2& du, const KVec2& dv) const
{
	assert(mpData);
	TextureFilter filter;
	return filter.SampleEWA((BitmapData*)mpData, uv, du, dv);
}

UINT32 Mipmap2D::GetSupportedSampleMode() const
{
	return (Tex2D::eEWA | Tex2D::eTrilinear | Tex2D::eBilinear);
}

UINT32 Mipmap2D::GetLevelCnt() const
{
	if (mpData)
		return (UINT32)((BitmapData*)mpData)->levels();
	else
		return 0;
}

UINT32 Mipmap2D::GetWidth() const
{
	if (mpData)
		return (UINT32)((BitmapData*)mpData)->size();
	else
		return 0;
}

UINT32 Mipmap2D::GetHeight() const
{
	if (mpData)
		return (UINT32)((BitmapData*)mpData)->size();
	else
		return 0;
}

Image2D::Image2D()
{
	mpData = NULL;
}

Image2D::~Image2D()
{
	if (mpData)
		delete (BitmapData*)mpData;
	mpData = NULL;
}

bool Image2D::SetSourceFile(const char* filename)
{
	if (mpData)
		delete (BitmapData*)mpData;

	mpData = new BitmapData(filename, false);
	if (!((BitmapData*)mpData)->IsOk()) {
		delete (BitmapData*)mpData;
		mpData = NULL;
		return false;
	}
	else
		return true;
}

KVec4 Image2D::SamplePoint(const KVec2& uv) const
{
	assert(mpData);
	TextureFilter filter;
	return filter.SampleBilinear((BitmapData*)mpData, uv);
}

KVec4 Image2D::SampleBilinear(const KVec2& uv) const
{
	assert(mpData);
	TextureFilter filter;
	return filter.SampleBilinear((BitmapData*)mpData, uv);
}

KVec4 Image2D::SampleTrilinear(const KVec2& uv, const KVec2& du, const KVec2& dv) const
{
	// No supported
	assert(false);
	return KVec4(0,0,0,0);
}

KVec4 Image2D::SampleEWA(const KVec2& uv, const KVec2& du, const KVec2& dv) const
{
	assert(mpData);
	TextureFilter filter;
	return filter.SampleFinestEWA((BitmapData*)mpData, uv, du, dv);
}

UINT32 Image2D::GetSupportedSampleMode() const
{
	return (Tex2D::eEWA | Tex2D::eBilinear);
}

UINT32 Image2D::GetLevelCnt() const
{
	return 1;
}

UINT32 Image2D::GetWidth() const
{
	if (mpData)
		return ((BitmapData*)mpData)->mActualWidth;
	else
		return 0;
}

UINT32 Image2D::GetHeight() const
{
	if (mpData)
		return ((BitmapData*)mpData)->mActualWidth;
	else
		return 0;
}

Procedure2D::Procedure2D()
{

}

Procedure2D::~Procedure2D()
{

}

KVec4 Procedure2D::SampleBilinear(const KVec2& uv) const
{
	// No supported
	assert(false);
	return KVec4(0,0,0,0);
}

KVec4 Procedure2D::SampleTrilinear(const KVec2& uv, const KVec2& du, const KVec2& dv) const
{
	// No supported
	assert(false);
	return KVec4(0,0,0,0);
}

KVec4 Procedure2D::SampleEWA(const KVec2& uv, const KVec2& du, const KVec2& dv) const
{
	// No supported
	assert(false);
	return KVec4(0,0,0,0);
}

UINT32 Procedure2D::GetSupportedSampleMode() const
{
	return Tex2D::ePoint;
}

UINT32 Procedure2D::GetLevelCnt() const
{
	return 1;
}

UINT32 Procedure2D::GetWidth() const
{
	return Tex2D::cInfinite;
}

UINT32 Procedure2D::GetHeight() const
{
	return Tex2D::cInfinite;
}


TextureManager* TextureManager::s_pInstance = NULL;

TextureManager::TextureManager()
{
	mSearchPaths.push_back("");
	mbUseMipmap = false;
}

TextureManager::~TextureManager()
{
	for (std_hash_map<std::string, Tex2D*>::iterator it = mBitmapTextures.begin(); it != mBitmapTextures.end(); ++it)
		delete it->second;
	mBitmapTextures.clear();
}

Tex2D* TextureManager::CreateBitmapTexture(const char* filename)
{
	if (mBitmapTextures.find(filename) != mBitmapTextures.end())
		return mBitmapTextures[filename];

	Tex2D* pMap = NULL;
	for (int i = (int)mSearchPaths.size() - 1; i >= 0 ; --i) {
		std::string path_name = mSearchPaths[i] + filename;
		if (mbUseMipmap) {
			Mipmap2D* res = new Mipmap2D();
			if (res->SetSourceFile(path_name.c_str()))
				pMap = res;
			else
				delete res;
		}
		else {
			Image2D* res = new Image2D();
			if (res->SetSourceFile(path_name.c_str()))
				pMap = res;
			else
				delete res;
		}
		if (pMap)
			break;
	}
	if (pMap == NULL)
		return NULL;

	mBitmapTextures[filename] = pMap;
	return pMap;
}

TextureManager* TextureManager::GetInstance()
{
	return s_pInstance;
}

void TextureManager::Initialize()
{
	s_pInstance = new TextureManager;
	Texture::TextureFilter::Initialize();
}

void TextureManager::Shutdown()
{
	Texture::TextureFilter::ShutDown();
	delete s_pInstance;
	s_pInstance = NULL;
}

void TextureManager::AddSearchPath(const char* path)
{
	std::string temp(path);
	if (!temp.empty() && temp[temp.size()-1] != '\\' && temp[temp.size()-1] != '/')
		temp += "/";
	mSearchPaths.push_back(temp);

}

} // Texture