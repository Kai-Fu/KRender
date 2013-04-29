#include "BitmapObject.h"
#include "color.h"
#include <stdio.h>
#include <memory.h>
#include <assert.h>
#include <FreeImage.h>


BitmapObject::BitmapObject(void)
{
	mpData = NULL;
	mAutoFreeMem = false;
}

BitmapObject::~BitmapObject(void)
{
	if (mAutoFreeMem)
		delete[] mpData;
}

void BitmapObject::SetBitmapData(void* pData, PixelFormat format, UINT32 w, UINT32 h, bool auto_free_mem)
{
	mpData = (unsigned char*)pData;
	
	mFormat = format;
	mWidth = w;
	mHeight = h;

	mAutoFreeMem = auto_free_mem;
}

void BitmapObject::SetPixel(void* pPixel, UINT32 x, UINT32 y)
{
	unsigned char* pDstPixel = mpData + y * mPitch + x * GetBPPByFormat(mFormat);
	memcpy(pDstPixel, pPixel, GetBPPByFormat(mFormat));
}

void* BitmapObject::GetPixel(UINT32 x, UINT32 y)
{
	return mpData + y * mPitch + x * GetBPPByFormat(mFormat);
}

const void* BitmapObject::GetPixel(UINT32 x, UINT32 y) const
{
	return mpData + y * mPitch + x * GetBPPByFormat(mFormat);
}

void BitmapObject::CopyFrom(const BitmapObject& src, 
		UINT32 dstX, UINT32 dstY,
		UINT32 srcX, UINT32 srcY,
		UINT32 w, UINT32 h)
{
	if (mFormat != src.mFormat ||
		dstX >= mWidth || dstY >= mHeight ||
		srcX >= src.mWidth || srcY >= src.mHeight)
		return; // bad parameters, kick out.
	w = (dstX + w) > mWidth ? (mWidth - dstX) : w;
	for (UINT32 y = dstY; y < dstY + h && y < mHeight; ++y) {
		UINT32 bpp = GetBPPByFormat(mFormat);
		memcpy(&mpData[y*mPitch + dstX*bpp], 
			src.GetPixel(srcX, srcY + y -dstY), 
			w*bpp);
	}
}

void BitmapObject::CopyFromRGB32F(const BitmapObject& src, 
		UINT32 dstX, UINT32 dstY,
		UINT32 srcX, UINT32 srcY,
		UINT32 w, UINT32 h)
{
	if (dstX >= mWidth || dstY >= mHeight ||
		srcX >= src.mWidth || srcY >= src.mHeight)
		return; // bad parameters, kick out.

	w = (dstX + w) > mWidth ? (mWidth - dstX) : w;
	UINT32 bpp = GetBPPByFormat(mFormat);
	for (UINT32 y = dstY; y < dstY + h && y < mHeight; ++y) {

		for (UINT32 x = dstX; x < dstX + w; ++x) {
			KColor* pColor = (KColor*)src.GetPixel(srcX + x - dstX, srcY + y -dstY);
			pColor->ConvertToDWORD(*(DWORD*)&mpData[y*mPitch + x*bpp]);
		}
	}
}

void BitmapObject::ClearByChecker()
{
	BYTE* pSrc = mpData;
	for (UINT32 i = 0; i < mHeight; ++i) {
		for (UINT32 j = 0; j < mWidth; ++j) {
			DWORD* pixel = (DWORD*)(pSrc + i * mWidth * 4 + j * 4);
			UINT32 det = (i >> 5) + (j >> 5);
			if (det & 0x01)
				*pixel = 0x0000ff00;
			else
				*pixel = 0x00ffffff;
		}
	}

}

void BitmapObject::ClearByZero()
{
	memset(mpData, 0, mPitch*mHeight);
}

BitmapObject* BitmapObject::CreateBitmap(UINT32 w, UINT32 h, PixelFormat format)
{
	BitmapObject* pBmp = new BitmapObject;
	pBmp->mFormat = format;

	BYTE* data = new BYTE[w * h * GetBPPByFormat(pBmp->mFormat)];
	pBmp->SetBitmapData(data, format, w, h, true);
	pBmp->mPitch = w * GetBPPByFormat(pBmp->mFormat);
	return pBmp;
}

UINT32 BitmapObject::GetBPPByFormat(PixelFormat format)
{
	switch (format) {
	case eRGBA8:
		return 4;
	case eRGB8:
		return 3;
	case eRGB32F:
		return 12;
	case eRGBA32F:
		return 16;
	case eR32F:
		return 4;
	default:
		return 0;
	}
}

bool BitmapObject::Save(const char* filename)
{
	if (!mpData)
		return false;

	FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;

	std::string pathname(filename);
	size_t dot_pos = pathname.rfind('.');
	const char* ext = NULL;
	if (dot_pos != std::string::npos) {
		ext = &pathname.c_str()[dot_pos + 1];
		fif = FreeImage_GetFIFFromFormat(ext);
	}
	else
		fif = FIF_PNG;

	
	bool result = false;

	if((fif != FIF_UNKNOWN) && FreeImage_FIFSupportsWriting(fif)) {
		FIBITMAP *dib = NULL;

		UINT32 bpp = GetBPPByFormat(mFormat);
		dib = FreeImage_ConvertFromRawBits(mpData, mWidth, mHeight, mWidth * bpp, bpp * 8, 0x000000ff, 0x0000ff00, 0x00ff0000);
		if (dib && FreeImage_Save(fif, dib, filename))
			result = true;
		FreeImage_Unload(dib);
	}

	return result;
}


bool BitmapObject::Load(const char* filename, std::vector<BitmapObject*>* mipmap_list)
{
	if (!mpData)
		return false;

	FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;
	fif = FreeImage_GetFileType(filename);

	if(fif == FIF_UNKNOWN)
		fif  =  FreeImage_GetFIFFromFilename(filename);

	if((fif != FIF_UNKNOWN) && FreeImage_FIFSupportsReading(fif)) {
		FIBITMAP *dib = FreeImage_Load(fif, filename);
		if (!dib) return false;

		dib = FreeImage_ConvertTo32Bits(dib);
		if (!dib) return false;

		mFormat = BitmapObject::eRGBA8;
		BYTE* bits = FreeImage_GetBits(dib);
		mWidth = FreeImage_GetWidth(dib);
		mHeight = FreeImage_GetHeight(dib);
		mPitch = FreeImage_GetPitch(dib);

		size_t dataSize = mWidth * mHeight * GetBPPByFormat(mFormat);
		mpData = new BYTE[dataSize];
		memcpy(mpData, bits, dataSize);

		// Generate the mipmap chain
		if (mipmap_list) {

			UINT32 w = mWidth / 2;
			UINT32 h = mHeight / 2;
			while (w > 0 || h > 0) {

				UINT32 ww = w > 1 ? w : 1;
				UINT32 hh = h > 1 ? h : 1;
				BitmapObject* pLevel = CreateBitmap(ww, hh, mFormat);

				FIBITMAP* oldDib = dib;
				dib = FreeImage_Rescale(dib, ww, hh, FILTER_BOX);
				FreeImage_Unload(oldDib);
				if (!dib) return false;

				UINT32 bpp = GetBPPByFormat(mFormat);
				BYTE* data = new BYTE[ww * hh * bpp];
				bits = FreeImage_GetBits(dib);
				memcpy(data, bits, bpp*ww*hh);
				pLevel->SetBitmapData(data, mFormat, ww, hh, true);
				mipmap_list->push_back(pLevel);

				w >>= 1;
				h >>= 1;
			}

		}
		FreeImage_Unload(dib);
		return true;
	}
	else
		return false;
}

