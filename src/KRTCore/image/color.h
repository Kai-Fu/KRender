#pragma once
#include <common/defines/typedefs.h>


struct KColor
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
	void ConvertToBYTE3(BYTE clr[3]) const;
	void ConvertFromBYTE3(BYTE clr[3]);

	float DiffRatio(const KColor& dst) const;
};

struct KPixel
{
	KColor color;
	float alpha;
};
