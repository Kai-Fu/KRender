/*
 * txfilt.cpp
 *
 * Copyright (C) 1999, Matt Pharr <mmp@graphics.stanford.edu>
 *
 * This software is placed in the public domain and is provided as is
 * without express or implied warranty.
 *
 * Various texture filtering routines; most notably Heckbert's EWA.
 * Code optimization possibilities abound.
 */

#include "txfilt.h"
#include <string.h>
#include <math.h>

// warning C4244: 'argument' : conversion from 'double' to 'float', possible loss of data
#pragma warning(disable:4244)
// Various utility functions from here and there that make things nicer.
namespace Texture {

template <class T, class U, class V> inline T
clamp(const T &val, const U &min, const V &max)
{
    if (val < min)
	return min;
    else if (val > max)
	return max;
    else
	return val;
}

static inline KVec4
lerp(float t, const KVec4 &p1, const KVec4 &p2) 
{
	return nvmath::lerp(t, p1, p2);
}

// Like a%b, but does the right thing when one or both is negative.
// From Darwyn Peachey's chapter in the Texturing and Modelling: A
// Procedural Approach book.

static int
mod(int a, int b) 
{
    int n = int(a/b);
    a -= n*b;
    if (a < 0)
        a += b;
    return a;
}


static inline int clamp(int v, int a, int b)
{
	if (v < a) return a;
	if (v > b) return b;
	return v;
}

inline float
max(float a, float b, float c)
{
    if (a >= b) {
	if (a >= c) return a;
	else return c;
    }
    else {
	if (b >= c) return b;
	else return c;
    }
}

inline float frac(float f) 
{
    return f >= 0. ? f - int(f) : frac(-f); 
}

////////////////
// TextureFilter

// Some interesting statistics.  statsTooEccentric counts the number of times
// an ellipse in texture space is squashed since it's so anisotropic that
// we'd have to access way too many texels to filter it correctly; in this
// case we blur it out a little bit in the interests of running in finite time.
// statsTexelsUsed counts the total number of texels accessed.
static int statsTooEccentric;

float *TextureFilter::weightLut = 0;

void TextureFilter::Initialize()
{
	weightLut = new float[WEIGHT_LUT_SIZE];

	for (int i = 0; i < WEIGHT_LUT_SIZE; ++i) {
		float alpha = 2;
		float r2 = float(i) / float(WEIGHT_LUT_SIZE - 1);
		float weight = exp(-alpha * r2);
		weightLut[i] = weight;
	}
}

void TextureFilter::ShutDown()
{
	delete[] weightLut;
	weightLut = NULL;
}


TextureFilter::TextureFilter(float lb)
{
    lodBias = lb;
}

KVec4
TextureFilter::SampleEWA(const TextureMap *map, const KVec2 &tex, 
		     const KVec2 &du, const KVec2 &dv) const 
{
	return ewa(map, tex, du, dv);
}

KVec4 
TextureFilter::SampleFinestEWA(const TextureMap *map, const KVec2 &pt, 
		   const KVec2 &du, const KVec2 &dv) const
{
	return ewaLod(map, pt, du, dv, 0);
}

KVec4 TextureFilter::SampleBilinear(const TextureMap *map, const KVec2 &pt) const
{
	return bilerpFinest(map, pt);
}

KVec4 TextureFilter::SampleBilinear_BorderClamp(const TextureMap *map, const KVec2 &pt) const
{
	return bilerpFinest_borderClamp(map, pt);
}

KVec4 TextureFilter::SampleTrilinear(const TextureMap *map, const KVec2 &pt, const KVec2 &du, const KVec2 &dv) const
{
	return mipmap(map, pt, du, dv);
}

static inline int FloatToInt(float x)
{
	unsigned e = (0x7F + 31) - ((* (unsigned*) &x & 0x7F800000) >> 23);
	unsigned m = 0x80000000 | (* (unsigned*) &x << 8);
	return int((m >> e) & -(e < 32));
}

void
TextureFilter::getFourTexels(const TextureMap *map, const KVec2 &p, 
			     KVec4 texels[2][2], int lod,
			     int ures, int vres) const
{
    // Note that here (and elsewhere), we assume that texture maps repeat;
    // if we wanted to (say) clamp at the edges, this and other code would
    // need to be generalized.
    int u0 = mod(FloatToInt(p[0]),   ures);
    int u1 = mod(FloatToInt(p[0])+1, ures);
    int v0 = mod(FloatToInt(p[1]),   vres);
    int v1 = mod(FloatToInt(p[1])+1, vres);

    texels[0][0] = map->texel(u0, v0, lod);
    texels[1][0] = map->texel(u1, v0, lod);
    texels[0][1] = map->texel(u0, v1, lod);
    texels[1][1] = map->texel(u1, v1, lod);
}

KVec4
TextureFilter::bilerpFinest(const TextureMap *map, const KVec2 &tex) const
{
    // Just do bilinear interpolation at the finest level of the map.
 
    KVec2 p(tex[0] * map->size(), tex[1] * map->size());

    float du = frac(p[0]);
    float dv = frac(p[1]);

    int u0 = mod(FloatToInt(p[0]),   map->size());
    int u1 = mod(FloatToInt(p[0])+1, map->size());
    int v0 = mod(FloatToInt(p[1]),   map->size());
    int v1 = mod(FloatToInt(p[1])+1, map->size());


    return lerp(dv, lerp(du, map->texel(u0, v0), map->texel(u1, v0)),
		    lerp(du, map->texel(u0, v1), map->texel(u1, v1)));
}

KVec4 
TextureFilter::bilerpFinest_borderClamp(const TextureMap *map, const KVec2 &tex) const
{
	 KVec2 p(tex[0] * map->size(), tex[1] * map->size());

    float du = frac(p[0]);
    float dv = frac(p[1]);

    int u0 = clamp(FloatToInt(p[0]),   0, map->size());
    int u1 = clamp(FloatToInt(p[0])+1, 0, map->size());
    int v0 = clamp(FloatToInt(p[1]),   0, map->size());
    int v1 = clamp(FloatToInt(p[1])+1, 0, map->size());


    return lerp(dv, lerp(du, map->texel(u0, v0), map->texel(u1, v0)),
		    lerp(du, map->texel(u0, v1), map->texel(u1, v1)));
}

static float invLog2 = 1. / log(2.);

#define LOG2(x) (log(x)*invLog2)

KVec4
TextureFilter::mipmap(const TextureMap *map, const KVec2 &tex, const KVec2 &duv,
		      const KVec2 &dvv) const
{
    KVec4 l0, l1;
    KVec4 texels[2][2];

    // Avoid the sqrts in length_squared() by multiplying the log by .5.
	float lod = -0.5f * LOG2(max(nvmath::lengthSquared(duv), nvmath::lengthSquared(dvv),
			       1e-6f));
    lod -= lodBias;

    if (lod >= map->levels() - 1)
	return bilerpFinest(map, tex);

    int lod0 = FloatToInt(lod);
    int lod1 = lod0 + 1;
    int scale0 = 1 << lod0;
    int scale1 = 1 << lod1;

    // lod 0
    KVec2 p(tex[0] * scale0, tex[1] * scale0);
    getFourTexels(map, p, texels, lod0, scale0, scale0);

    float du = frac(p[0]);
    float dv = frac(p[1]);
    l0 = lerp(dv, lerp(du, texels[0][0], texels[1][0]),
	          lerp(du, texels[0][1], texels[1][1]));

    // lod 1
    p = KVec2(tex[0] * scale1, tex[1] * scale1);
    getFourTexels(map, p, texels, lod1, scale1, scale1);

    du = frac(p[0]);
    dv = frac(p[1]);
    l1 = lerp(dv, lerp(du, texels[0][0], texels[1][0]),
	          lerp(du, texels[0][1], texels[1][1]));

    return lerp(frac(lod), l0, l1);
}

// Heckbert's elliptical weighted average filtering

KVec4
TextureFilter::ewa(const TextureMap *map, const KVec2 &tex, const KVec2 &du,
		   const KVec2 &dv) const
{
    KVec2 major, minor;
    
	if (nvmath::lengthSquared(du) < nvmath::lengthSquared(dv)) {
	major = dv;
	minor = du;
    }
    else {
	major = du;
	minor = dv;
    }

	float majorLength = nvmath::length(major);
	float minorLength = nvmath::length(minor);

    // if the eccentricity of the ellipse is looking to be too big, scale
    // up the shorter of the two vectors so that it's a little more reasonable.
    // This lets us avoid spending inordinate amounts of time filtering very
    // long and skinny regions (which take a lot of time), at the expense of
    // some blurring...
    const float maxEccentricity = 30;
    float e = majorLength / minorLength;

    if (e > maxEccentricity) {
	++statsTooEccentric;

	// blur in the interests if filtering in a reasonable amount
	// of time
	minor = minor * (e / maxEccentricity);
	minorLength *= e / maxEccentricity;
    }

    // Pick a lod such that we're looking at somewhere around 3-9 texels
    // in the minor axis direction.
    float lod = LOG2(3. / minorLength) - lodBias;
    lod = clamp(lod, 0., map->levels() - 1 - 1e-7);

    // avoid spending lots of time on filtering enormous regions and just
    // return the average value.
    if (lod == 0.) {
	return map->texel(0, 0, 0);
    }
    else {
	// don't bother interpolating between multiple LODs; it doesn't seem to
	// be worth the extra running time.
	return ewaLod(map, tex, du, dv, floor(lod));
    }
}

KVec4
TextureFilter::ewaLod(const TextureMap *map, const KVec2 &texO, const KVec2 &du,
		      const KVec2 &dv, int lod) const
{
    int scale = 1 << lod;

	float nu = texO[0] - floor(texO[0]);
	float nv = texO[1] - floor(texO[1]);
	KVec2 tex(abs(nu) * scale, abs(nv) * scale);

    float ux = du[0] * scale;
    float vx = du[1] * scale;
    float uy = dv[0] * scale;
    float vy = dv[1] * scale;

    // compute ellipse coefficients to bound the region: 
    // A*x*x + B*x*y + C*y*y = F.
    float A = vx*vx+vy*vy+1;
    float B = -2*(ux*vx+uy*vy);
    float C = ux*ux+uy*uy+1;
    float F = A*C-B*B/4.;

    // it better be an ellipse!
    assert(A*C-B*B/4. > 0.);

    // Compute the ellipse's (u,v) bounding box in texture space
    int u0 = floor(tex[0] - 2. / (-B*B+4.0*C*A) * sqrt((-B*B+4.0*C*A)*C*F));
    int u1 = ceil (tex[0] + 2. / (-B*B+4.0*C*A) * sqrt((-B*B+4.0*C*A)*C*F));
    int v0 = floor(tex[1] - 2. / (-B*B+4.0*C*A) * sqrt(A*(-B*B+4.0*C*A)*F));
    int v1 = ceil (tex[1] + 2. / (-B*B+4.0*C*A) * sqrt(A*(-B*B+4.0*C*A)*F));

    // Heckbert MS thesis, p. 59; scan over the bounding box of the ellipse
    // and incrementally update the value of Ax^2+Bxy*Cy^2; when this
    // value, q, is less than F, we're inside the ellipse so we filter
    // away..
    KVec4 num(0, 0, 0, 0);
    float den = 0;
    float ddq = 2 * A;
    float U = u0 - tex[0];
    for (int v = v0; v <= v1; ++v) {
	float V = v - tex[1];
	float dq = A*(2*U+1) + B*V;
	float q = (C*V + B*U)*V + A*U*U;

	for (int u = u0; u <= u1; ++u) {
	    if (q < F) {

		float r2 = q / F;
		float weight = getWeight(r2);

		num += map->texel(mod(u, scale), mod(v, scale), lod) * weight;
		den += weight;
	    }
	    q += dq;
	    dq += ddq;
	}
    }

    assert(den > 0.);
    return num / den;
}

} // namespace Texture
#pragma warning(default:4244)