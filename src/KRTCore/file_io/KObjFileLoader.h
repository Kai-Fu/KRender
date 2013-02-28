#pragma once
#include "../base/geometry.h"
#include "glm.h"
#include <vector>
#include <string>
#include <hash_map>

class KObjFileLoader
{
public:
	bool mUseTexMap;
	bool mAdjustObjCenter;


	KBBox mObjBBox;
public:
	KObjFileLoader(void);
	virtual ~KObjFileLoader(void);

	bool LoadObjFile(const char* filename, KScene& scene, KVec3& scene_offset, float& scene_scale);

protected:
	virtual void ProcessingMaterial(GLMmodel* model, KScene& scene, const stdext::hash_map<UINT32, UINT32>& nodeToMtl) = 0;
	virtual void GetNodeLocalMatrix(const char* nodeName, KTriMesh* pMesh, KMatrix4& outMat);
};
