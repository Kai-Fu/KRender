#include "../math/Vecnt.h"
#include "../math/Matnnt.h"

typedef nvmath::Vecnt<4, float> KVec4;
typedef nvmath::Vecnt<3, float> KVec3;
typedef nvmath::Vecnt<2, float> KVec2;

typedef nvmath::Vecnt<4, double> KVec4d;
typedef nvmath::Vecnt<3, double> KVec3d;
typedef nvmath::Vecnt<2, double> KVec2d;

typedef nvmath::Vecnt<2, int>	KVecInt2;

typedef nvmath::Quatf KQuat;
typedef nvmath::Quatd KQuatd;

typedef nvmath::Mat44f KMatrix4;
typedef nvmath::Mat44d KMatrix4d;

typedef unsigned char	BYTE;
typedef unsigned long	DWORD;
typedef unsigned int	UINT32;
typedef unsigned short  UINT16;


#ifdef _WIN32
	typedef unsigned __int64 UINT64;
#endif