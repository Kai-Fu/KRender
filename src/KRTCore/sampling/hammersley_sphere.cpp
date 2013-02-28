#include "hammersley_sphere.h"

namespace Sampling {

void HammersleySphere(KVec3* result, int start, int n)
{
	float p, t, st, phi, phirad;
	int k, kk, pos;
	for (k=start, pos=0 ; k<start+n ; k++)
	{
		t = 0;
		for (p=0.5, kk=k ; kk ; p*=0.5, kk>>=1)
			if (kk & 1) // kk mod 2 == 1
				t += p;
		t = 2.0f * t - 1.0f; // map from [0,1] to [-1,1]
		phi = (k + 0.5f) / n; // a slight shift
		phirad = phi * 2.0f * nvmath::PI; // map to [0, 2 pi)
		st = sqrt(1.0f-t*t);
		result[pos][0] = st * cos(phirad);
		result[pos][1] = st * sin(phirad);
		result[pos][2] = t;
		++pos;
	}
}

}