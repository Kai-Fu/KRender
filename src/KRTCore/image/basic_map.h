#pragma once

#include "color.h"
#include "BitmapObject.h"
#include <vector>
#include <common/defines/stl_inc.h>

namespace Texture {


class Tex2D
{
public:
	enum SampleMode {ePoint = 0x01, eBilinear = 0x02, eTrilinear = 0x04, eEWA = 0x08};
	static const UINT32 cInfinite = 0xffffffff;

	virtual KVec4 SamplePoint(const KVec2& uv) const = 0;
	virtual KVec4 SampleBilinear(const KVec2& uv) const = 0;
	virtual KVec4 SampleTrilinear(const KVec2& uv, const KVec2& du, const KVec2& dv) const = 0;
	virtual KVec4 SampleEWA(const KVec2& uv, const KVec2& du, const KVec2& dv) const = 0;
	virtual UINT32 GetSupportedSampleMode() const = 0;

	virtual UINT32 GetLevelCnt() const = 0;
	virtual UINT32 GetWidth() const = 0;
	virtual UINT32 GetHeight() const = 0;
};

class Mipmap2D : public Tex2D
{
public:
	Mipmap2D();
	~Mipmap2D();
	bool SetSourceFile(const char* filename);

	virtual KVec4 SamplePoint(const KVec2& uv) const;
	virtual KVec4 SampleBilinear(const KVec2& uv) const;
	virtual KVec4 SampleTrilinear(const KVec2& uv, const KVec2& du, const KVec2& dv) const;
	virtual KVec4 SampleEWA(const KVec2& uv, const KVec2& du, const KVec2& dv) const;
	virtual UINT32 GetSupportedSampleMode() const;

	virtual UINT32 GetLevelCnt() const;
	virtual UINT32 GetWidth() const;
	virtual UINT32 GetHeight() const;

private:
	void* mpData;

};

class Image2D : public Tex2D
{
public:
	Image2D();
	~Image2D();
	bool SetSourceFile(const char* filename);

	virtual KVec4 SamplePoint(const KVec2& uv) const;
	virtual KVec4 SampleBilinear(const KVec2& uv) const;
	virtual KVec4 SampleTrilinear(const KVec2& uv, const KVec2& du, const KVec2& dv) const;
	virtual KVec4 SampleEWA(const KVec2& uv, const KVec2& du, const KVec2& dv) const;

	virtual UINT32 GetSupportedSampleMode() const;

	virtual UINT32 GetLevelCnt() const;
	virtual UINT32 GetWidth() const;
	virtual UINT32 GetHeight() const;

	KVec4 SampleBilinear_BorderClamp(const KVec2& uv) const;

private:
	void* mpData;
};

class Procedure2D : public Tex2D
{
public:
	Procedure2D();
	~Procedure2D();

	// The derived classes need to implement this functions
	// virtual KColor SamplePoint(const KVec2& uv) const;

	virtual KVec4 SampleBilinear(const KVec2& uv) const;
	virtual KVec4 SampleTrilinear(const KVec2& uv, const KVec2& du, const KVec2& dv) const;
	virtual KVec4 SampleEWA(const KVec2& uv, const KVec2& du, const KVec2& dv) const;
	virtual UINT32 GetSupportedSampleMode() const;

	virtual UINT32 GetLevelCnt() const;
	virtual UINT32 GetWidth() const;
	virtual UINT32 GetHeight() const;
};

class TexCube
{
public:
	enum SampleMode {ePoint = 0x01, eBilinear = 0x02 };
	enum FaceID {eLeft = 0, eFront, eRight, eBack, eTop, eBottom};
	TexCube();
	~TexCube();
	bool SetSourceFile(const char* filename);

	KVec4 SamplePoint(const KVec3& uvw) const;
	KVec4 SampleBilinear(const KVec3& uvw) const;

protected:
	void GetFaceID_UV(const KVec3& uvw, Image2D* &face, KVec2& uv) const;
private:
	Image2D* mFaceTex[6];
};

class TextureManager
{
public:
	TextureManager();
	~TextureManager();

	Tex2D* CreateBitmapTexture(const char* filename);
	void AddSearchPath(const char* path);

	static TextureManager* GetInstance();
	static void Initialize();
	static void Shutdown();

private:
	std_hash_map<std::string, Tex2D*> mBitmapTextures;
	std::vector<std::string> mSearchPaths;
	bool mbUseMipmap;

	static TextureManager* s_pInstance;

};


} // namespace Generator