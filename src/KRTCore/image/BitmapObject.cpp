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

void BitmapObject::SetBitmapData(void* pData, 
	UINT32 bpp, UINT32 pitch,
	UINT32 channel_cnt,
	UINT32 w, UINT32 h,
	bool auto_free_mem)
{
	mpData = (unsigned char*)pData;
	mBpp = bpp;
	mPitch = pitch;

	mWidth = w;
	mHeight = h;

	mChannelCnt = channel_cnt;
	mBitsPerChannel = mBpp / channel_cnt * 8;
	mIsFixedPoint = (mBitsPerChannel == 32) ? false : true;
	mAutoFreeMem = auto_free_mem;
}

void BitmapObject::SetPixel(void* pPixel, UINT32 x, UINT32 y)
{
	unsigned char* pDstPixel = mpData + y * mPitch + x * mBpp;
	memcpy(pDstPixel, pPixel, mBpp);
}

void* BitmapObject::GetPixel(UINT32 x, UINT32 y)
{
	return mpData + y * mPitch + x * mBpp;
}

const void* BitmapObject::GetPixel(UINT32 x, UINT32 y) const
{
	return mpData + y * mPitch + x * mBpp;
}

void BitmapObject::CopyFrom(const BitmapObject& src, 
		UINT32 dstX, UINT32 dstY,
		UINT32 srcX, UINT32 srcY,
		UINT32 w, UINT32 h)
{
	if (mBpp != src.mBpp ||
		dstX >= mWidth || dstY >= mHeight ||
		srcX >= src.mWidth || srcY >= src.mHeight)
		return; // bad parameters, kick out.
	w = (dstX + w) > mWidth ? (mWidth - dstX) : w;
	for (UINT32 y = dstY; y < dstY + h && y < mHeight; ++y) {

		memcpy(&mpData[y*mPitch + dstX*mBpp], 
			src.GetPixel(srcX, srcY + y -dstY), 
			w*mBpp);
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
	for (UINT32 y = dstY; y < dstY + h && y < mHeight; ++y) {

		for (UINT32 x = dstX; x < dstX + w; ++x) {
			KColor* pColor = (KColor*)src.GetPixel(srcX + x - dstX, srcY + y -dstY);
			pColor->ConvertToDWORD(*(DWORD*)&mpData[y*mPitch + x*mBpp]);
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

	UINT32 bpp = 0;
	UINT32 channel_cnt = 0;
	switch (format) {
	case eRGBA8:
		bpp = 4;
		channel_cnt = 4;
		break;
	case eRGB8:
		bpp = 3;
		channel_cnt = 3;
		break;
	case eRGB32F:
		bpp = 4 * 3;
		channel_cnt = 3;
		break;
	case eRGBA32F:
		bpp = 4 * 4;
		channel_cnt = 4;
		break;
	case eR32F:
		channel_cnt = 1;
		bpp = 4;
		break;
	}
	BYTE* data = new BYTE[w * h * bpp];
	pBmp->SetBitmapData(data, bpp, w * bpp, channel_cnt, w, h, true);
	return pBmp;
}

bool BitmapObject::Save(const char* filename)
{
	if (!mpData)
		return false;

	FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;
	fif = FreeImage_GetFileType(filename);
	bool result = false;
	if(fif == FIF_UNKNOWN)
		fif  =  FreeImage_GetFIFFromFilename(filename);

	if((fif != FIF_UNKNOWN) && FreeImage_FIFSupportsReading(fif)) {
		FIBITMAP *dib = NULL;

		dib = FreeImage_AllocateT(FIT_BITMAP, mWidth, mHeight, mBpp);
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

		BYTE* bits = FreeImage_GetBits(dib);
		mWidth = FreeImage_GetWidth(dib);
		mHeight = FreeImage_GetHeight(dib);
		mPitch = FreeImage_GetPitch(dib);
		mBpp = FreeImage_GetBPP(dib);
		assert(mBpp == 4);

		mChannelCnt = 4;
		mBitsPerChannel = 8;
		size_t dataSize = mWidth * mHeight * mBpp;
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

				BYTE* data = new BYTE[ww * hh * mBpp];
				bits = FreeImage_GetBits(dib);
				memcpy(data, bits, mBpp*ww*hh);
				pLevel->SetBitmapData(data, mBpp, ww * mBpp, mChannelCnt, ww, hh, true);
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

