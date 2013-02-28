#include "halton2d.h"
#include <common/math/nvmath.h>

namespace Sampling {

void Halton2D(KVec2* result, int start, int n, const KVec2& warp, const KVec2& min, const KVec2& max, int p2/* = 3*/)
{
	float p, u, v, ip;
	int k, kk, pos, a;
	n += start;
	for (k=start, pos=0 ; k<n ; k++)
	{
		u = 0;
		for (p=0.5, kk=k ; kk ; p*=0.5, kk>>=1)
			if (kk & 1) // kk mod 2 == 1
				u += p;
		v = 0;
		ip = 1.0f/p2; // inverse of p2
		for (p=ip, kk=k ; kk ; p*=ip, kk/=p2) // kk = (int)(kk/p2)
			if ((a = kk % p2))
				v += a * p;

		u += warp[0];
		if (u > 1) u -= 1.0f;
		v += warp[0];
		if (v > 1) v -= 1.0f;
		result[pos][0] = nvmath::lerp(u, min[0], max[0]);
		result[pos][1] = nvmath::lerp(v, min[1], max[1]);
		pos++;
	}
}

}