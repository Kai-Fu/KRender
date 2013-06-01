/*
 * txfilt.h
 *
 * Copyright (C) 1999, Matt Pharr <mmp@graphics.stanford.edu>
 *
 * This software is placed in the public domain and is provided as is
 * without express or implied warranty.
 *
 * Some various ways of filtering given regions of texture maps; you need
 * some mechanism for computing the region to filter over on your own.  The
 * most interesting/high-quality of the filtering methods is Heckbert's
 * elliptical weighted average filter.
 */

/* [Kai@April/9/2009] I add texfilt.h/.cpp into my project, thank Matt Pharr:-)*/

#ifndef TXFILT_H
#define TXFILT_H

#include <math.h>
#include <common/defines/typedefs.h>
#include <assert.h>

namespace Texture {

// An interface that needs to be adhered to by the class that holds texture
// data.  It is assumed that texture maps are square and that the length of
// their sides are a power of two; if this is not the case, you need to
// resample your texture map so that this is true.

class TextureMap {
public:
    virtual ~TextureMap() { }

    // Returns a texel from the given level. 0 is the *least* detailed
    // level, consisting of a single pixel. The coordinates given will be
    // in the range [0, (1 << lod) - 1 ), etc.
	virtual KVec4 texel(int u, int v, int lod) const = 0;

    // Returns a texel from the most detailed level.
    virtual KVec4 texel(int u, int v) const = 0;

    // Returns the length of a side of the texture map.
    virtual int size() const = 0;

    // The number of texture map levels.  If it's not == log2(size()),
    // then one of the assumptions described above is not true.
    virtual int levels() const = 0;
};

// The texture filtering class that actually does the work.

class TextureFilter {
public:
    // filter should be "bilerp", "mipmap", or "anisotropic"; if it's none
    // of the above, Heckbert's anisotropic EWA filter is used.  "bilerp"
    // just does bilinear interpolation from the finest level of the
    // texture map, which will alias like mad (of course), and "mipmap"
    // does classic trilinear interpolation of two texture map levels.  The
    // lodBias parameter can be useful for making things a little sharper
    // (with the risk of aliasing) when using "mipmap" filtering.
    TextureFilter(float lodBias = 0.);

    // Given a texture map, a center point (pt), and two offset vectors
    // from pt that describe the area of texture map to be filtered over,
    // do a filtering calculation and return the result.
    KVec4 SampleEWA(const TextureMap *map, const KVec2 &pt, 
		   const KVec2 &du, const KVec2 &dv) const;

	KVec4 SampleFinestEWA(const TextureMap *map, const KVec2 &pt, 
		   const KVec2 &du, const KVec2 &dv) const;
	
	KVec4 SampleBilinear(const TextureMap *map, const KVec2 &pt) const;
	KVec4 SampleBilinear_BorderClamp(const TextureMap *map, const KVec2 &pt) const;

	KVec4 SampleTrilinear(const TextureMap *map, const KVec2 &pt, const KVec2 &du, const KVec2 &dv) const;


	static void Initialize();
	static void ShutDown();
private:
    float lodBias;

    KVec4 bilerpFinest(const TextureMap *map, const KVec2 &tex) const;
	KVec4 bilerpFinest_borderClamp(const TextureMap *map, const KVec2 &tex) const;

    KVec4 mipmap(const TextureMap *map, const KVec2 &tex, const KVec2 &du,
		    const KVec2 &dv) const;
    KVec4 ewa(const TextureMap *map, const KVec2 &tex, const KVec2 &du,
		 const KVec2 &dv) const;
    KVec4 ewaLod(const TextureMap *map, const KVec2 &tex, const KVec2 &du,
		    const KVec2 &dv, int lod) const;

    void getFourTexels(const TextureMap *map, const KVec2 &p,
		       KVec4 texels[2][2], int lod,
		       int ures, int vres) const;

    inline static float getWeight(float r2);
#define WEIGHT_LUT_SIZE 4096
    static float *weightLut;
};

inline float
TextureFilter::getWeight(float r2)
{
    int offset = (int)(r2 * (WEIGHT_LUT_SIZE - 1));
    //assert(offset >= 0 && offset < WEIGHT_LUT_SIZE);
	if (offset < 0) offset = 0;
	if (offset >= WEIGHT_LUT_SIZE) offset = WEIGHT_LUT_SIZE - 1;
    return weightLut[offset];
}

} // namespace Texture
#endif // TXFILT_H
