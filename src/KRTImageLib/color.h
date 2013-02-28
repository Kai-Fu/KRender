#pragma once
#include <common/defines/config.h>
#include <common/defines/typedefs.h>


struct KRT_IMAGE_API KColor
{
	float r, g, b;
	KColor() {r= 0; g = 0; b = 0;}
	KColor(float red, float green, float blue) {r = red; g = green; b = blue;}
	KColor(float* data) {r = data[0]; g = data[1]; b = data[2];}

	float Luminance() const  {return (r+g+b) / 3.0f;}
	void Add(const KColor& ref) {r += ref.r; g += ref.g; b += ref.b;}
	void Modulate(const KColor& ref) {r *= ref.r; g *= ref.g; b *= ref.b;}
	void Lerp(const KColor& ref, float value);
	void Lerp(const KColor& ref, const KColor& value);
	void Scale(float s) {r *= s; g *= s; b *= s;}
	void Clear() {r = g = b = 0;}
	void ConvertToDWORD(DWORD& clr) const;
	float DiffRatio(const KColor& dst) const;

};
