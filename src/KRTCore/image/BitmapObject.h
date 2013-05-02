#pragma once
#include <common/defines/typedefs.h>
#include "../image/color.h"

class BitmapObject
{
public:
	enum PixelFormat {
		eRGBA8,
		eRGB8,
		eRGB32F,
		eRGBA32F,
		eR32F
	};



	BitmapObject(void);
	~BitmapObject(void);


	void SetBitmapData(void* pData, PixelFormat format, UINT32 w, UINT32 h, bool auto_free_mem);

	void SetPixelData(void* pPixel, UINT32 x, UINT32 y);
	void* GetPixelPtr(UINT32 x, UINT32 y);
	const void* GetPixelPtr(UINT32 x, UINT32 y) const;

	KPixel GetPixel(UINT32 x, UINT32 y) const;
	void SetPixel(UINT32 x, UINT32 y, const KPixel& pix);
	
	void CopyFrom(const BitmapObject& src, 
		UINT32 dstX, UINT32 dstY,
		UINT32 srcX, UINT32 srcY,
		UINT32 w, UINT32 h);
	void CopyFromRGB32F(const BitmapObject& src, 
		UINT32 dstX, UINT32 dstY,
		UINT32 srcX, UINT32 srcY,
		UINT32 w, UINT32 h);
	void ClearByChecker();
	void ClearByZero();

	bool Save(const char* filename) const;
	bool Load(const char* filename, std::vector<BitmapObject*>* mipmap_list);

	static BitmapObject* CreateBitmap(UINT32 w, UINT32 h, PixelFormat format, void* pUserBuf = NULL);
	static UINT32 GetBPPByFormat(PixelFormat format);
public:

	PixelFormat mFormat;
	BYTE* mpData;
	UINT32 mPitch;
	UINT32 mWidth;
	UINT32 mHeight;

	bool mAutoFreeMem;

	
};

