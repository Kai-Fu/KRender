#include "BitmapObject.h"
#include "color.h"
#include <stdio.h>
#include <memory.h>
#include <assert.h>
#include <IL/il.h>
#include <IL/ilu.h>


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

	ILenum format, type;
	ILubyte channel_cnt;
	switch (mFormat) {
	case eRGBA8:
		format = IL_BGRA;
		type = IL_UNSIGNED_BYTE;
		channel_cnt = 4;
		break;
	case eRGB8:
		format = IL_BGR;
		type = IL_UNSIGNED_BYTE;
		channel_cnt = 3;
		break;
	case eRGB32F:
		format = IL_RGB;
		type = IL_FLOAT;
		channel_cnt = 3;
		break;
	case eRGBA32F:
		format = IL_RGBA;
		type = IL_FLOAT;
		channel_cnt = 4;
		break;
	case eR32F:
		format = IL_LUMINANCE;
		type = IL_FLOAT;
		channel_cnt = 1;
		break;
	default:
		assert(0);
	}

	ILuint image_id;
	image_id = ilGenImage();
	ilBindImage(image_id);

	ilEnable(IL_ORIGIN_SET);
	ilSetInteger(IL_ORIGIN_MODE, IL_ORIGIN_LOWER_LEFT);

	ilTexImage( mWidth, mHeight, 1, channel_cnt, format, type, mpData);
	bool result = ilSaveImage(filename) ? true : false;
	ilBindImage(0);
	ilDeleteImage(image_id);

	return result;
}


bool BitmapObject::Load(const char* filename, std::vector<BitmapObject*>* mipmap_list)
{
	ILuint image_id;
	image_id = ilGenImage();
	ilBindImage(image_id);

	ilEnable(IL_ORIGIN_SET);
	ilSetInteger(IL_ORIGIN_MODE, IL_ORIGIN_LOWER_LEFT);

	bool result = false;
	PixelFormat format;
	if (ilLoadImage(filename)) {
		ILint bpp = ilGetInteger(IL_IMAGE_BYTES_PER_PIXEL);
		ILint channel_cnt = 0;
		if (ilGetPalette()) 
			channel_cnt = ilGetInteger(IL_PALETTE_BPP);
		else
			channel_cnt = ilGetInteger(IL_IMAGE_CHANNELS);
		switch (channel_cnt) {
		case 4:  // this is RGBA format
			format = bpp > 4 ? eRGBA32F : eRGBA8;
			break;
		case 3:  // this is RGB format
			format = bpp > 3 ? eRGB32F : eRGB8;
			break;
		default: // assume it's R32F format
			format = eR32F;
			bpp = 1;
			break;
		}
		result = true;
	}
	else {
		ilBindImage(0);
		ilDeleteImage(image_id);
		return false;
	}
	
	ILenum dstFormat;
	ILenum dstType;
	UINT32 bpp = 0;
	UINT32 channel_cnt = 0;
	switch (format) {
	case eRGBA8:
		channel_cnt = 4;
		dstFormat = IL_BGRA;
		dstType = IL_UNSIGNED_BYTE;
		bpp = 4;
		break;
	case eRGB8:
		channel_cnt = 3;
		dstFormat = IL_BGR;
		dstType = IL_UNSIGNED_BYTE;
		bpp = 3;
		break;
	case eRGB32F:
		channel_cnt = 3;
		dstFormat = IL_RGB;
		dstType = IL_FLOAT;
		bpp = 3 * 4;
		break;
	case eRGBA32F:
		channel_cnt = 4;
		dstFormat = IL_RGBA;
		dstType = IL_FLOAT;
		bpp = 4 * 4;
		break;
	case eR32F:
		channel_cnt = 1;
		dstFormat = IL_LUMINANCE;
		dstType = IL_FLOAT;
		bpp = 4;
		break;
	default:
		assert(0);  // Shouldn't reach here
	}

	if (result && ilConvertImage(dstFormat, dstType)) {
		UINT32 w = (UINT32)ilGetInteger(IL_IMAGE_WIDTH);
		UINT32 h = (UINT32)ilGetInteger(IL_IMAGE_HEIGHT);
		mFormat = format;
		BYTE* data = new BYTE[w * h * bpp];
		memcpy(data, ilGetData(), bpp*w*h);
		SetBitmapData(data, bpp, w * bpp, channel_cnt, w, h, true);
	}
	else
		result = false;

	// Generate the mipmap chain
	if (mipmap_list) {

		UINT32 w = (UINT32)ilGetInteger(IL_IMAGE_WIDTH) / 2;
		UINT32 h = (UINT32)ilGetInteger(IL_IMAGE_HEIGHT) / 2;
		while (w > 0 || h > 0) {

			UINT32 ww = w > 1 ? w : 1;
			UINT32 hh = h > 1 ? h : 1;
			BitmapObject* pLevel = CreateBitmap(ww, hh, mFormat);

			iluScale(ww, hh, 1);
			BYTE* data = new BYTE[ww * hh * bpp];
			memcpy(data, ilGetData(), bpp*ww*hh);
			pLevel->SetBitmapData(data, bpp, ww * bpp, channel_cnt, ww, hh, true);
			mipmap_list->push_back(pLevel);

			w >>= 1;
			h >>= 1;
		}

	}

	ilBindImage(0);
	ilDeleteImage(image_id);

	return result;
}

