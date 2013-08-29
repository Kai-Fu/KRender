#pragma once
#include "BaseHeader.h"
#include "../image/color.h"
#include <vector>
#include <string>


// Force to align in 16 byte to improve cache coherency
#define INVALID_INDEX 0xffffffff
#define NOT_HIT_INDEX 0xfffffffe


class KMemCacher
{
public:
	KMemCacher(UINT32 bucketSize);
	~KMemCacher();

	void SetMemAllocAlignment(UINT32 alignment);
	void Reset();

	bool EntryExists(UINT64 id);
	void* LRU_OpenEntry(UINT64 id,  UINT32 memSize, bool& needUpdate);

protected:

	struct EntryDesc
	{
		UINT64 id;
		void* pMem;
		UINT32 memSize;

		UINT64 lru_flag;
	};

	static const UINT64 INVALID_ENTRY_IDX = 0xffffffffffffffff;
	UINT32 mAllocAlignment;
	std::vector<EntryDesc> mEntries;
	UINT64 mLRU_TimeStamp;

};

KVec3d ToVec3d(const KVec3& ref);
KVec3 ToVec3f(const KVec3d& ref);
KVec4d ToVec4d(const KVec4& ref);
KMatrix4d ToMatrix4d(const KMatrix4& ref);

struct ShadingContext;
class KBBox;

struct RayTriIntersect
{
	float ray_t;
	float u, v;
};

struct IntersectContext
{
	double ray_t;

	float w, u, v;
	UINT32 tri_id;
	UINT32 kd_leaf_idx;
	UINT32 bbox_node_idx;

	KVec3 walkVec;
	IntersectContext();
	void Reset();
};

class KRay 
{
private:
	KVec3d mOrign;
	KVec3d mDir;
public:

	KVec3d mRcpDir;
	KVec3d mNormDir;
	double mDirLen;
	int mSign[4];

	// triangle ID used to exclude the hit testing
	UINT32 mExcludeBBoxNode;
	UINT32 mExcludeTriID;

public:
	void Init(const KVec3d& o, const KVec3d& d, const KBBox* clamp_bbox);
	void InitTranslucentRay(const ShadingContext& shadingCtx, const KBBox* bbox);
	void InitReflectionRay(const ShadingContext& shadingCtx, const KVec3& in_dir, const KBBox* bbox);
	
	KVec3d GetOrg() const {return mOrign;}
	KVec3d GetDir() const {return mDir;}

	void Init(const KVec3d& o, const KVec3d& d);
};

class KTriDesc;
struct KTriVertPos2;


class KBSphere
{
public:
	KVec3 mCenter;
	float mRadius;

	KBSphere();
	KBSphere(const KBSphere& ref);

	void SetEmpty();

	bool IsEmpty() const;
	bool IsInside(const KVec3& vert) const;
};

class KBoxNormalizer
{
public:
	KVec3 mCenter;
	KVec3 mRcpScale;
	KVec3 mNormRcpScale;
	double mRcpScaleLen;

	void InitFromBBox(const KBBox& box);
	void ApplyToMatrix(KMatrix4& mat) const;
	void ApplyToRay(KVec3& rayOrg, KVec3& rayDir) const;
};

class KBBox
{
public:
	KVec3 mMin;
	KVec3 mMax;

	KBBox();
	KBBox(const KTriVertPos2& tri);
	const KVec3& operator[](int i) const {return i ? mMax : mMin;}
	void Add(const KBBox& bbox);
	void ContainVert(const KVec3& vert);
	void ClampBBox(const KBBox& bbox);
	void TransformByMatrix(const KMatrix4& mat);
	void TransformByMatrix(const KMatrix4d& mat);
	void SetEmpty();
	void Enlarge(float factor);

	bool IsEmpty() const;
	bool IsInside(const KVec3& vert) const;
	bool IsOverlapping(const KBSphere& sphere) const;
	const KVec3 Center() const;
	void GetFaceArea(float area[3]) const;
};


struct KTriangle {
	UINT32 pn_idx[3]; // position & normal index
};

struct KTexTriangle {
	UINT32 tt_idx[3];  // texcoord & tangent index
};

struct KTriVertPos1 {
	KVec3		mVertPos[3];
};

struct KTriVertPos2
{
	KVec3		mVertPos[3];
	KVec3		mVertPos_Delta[3];
	bool		mIsMoving;
};

// Data structure to accelerate ray-triangle intersection
class KTriDesc
{
public:
	UINT32	mNodeMeshIdx;
	UINT32	mTriIdx;

	UINT32 GetMeshIdx() const {return (mNodeMeshIdx & 0x0000ffff);}
	UINT32 GetNodeIdx() const {return ((mNodeMeshIdx & 0xffff0000) >> 16);}

	void SetNodeMeshIdx(UINT32 nodeIdx, UINT32 meshIdx) {mNodeMeshIdx = ((nodeIdx << 16) | (meshIdx & 0x0000ffff));}
};

class KTriMesh
{

public:
	std::vector<KTriangle> mFaces;
	std::vector<KTexTriangle> mTexFaces;
	struct PN_Data {
		KVec3 pos;
		KVec3 nor;
	};

	struct TT_Data {
		KVec2 texcoord;
		KVec3 tangent;
		KVec3 binormal;
	};

	bool mHasPNAnim;
	UINT32 mPN_VertCnt;

	bool mHasTTAnim;
	UINT32 mTT_VertCnt;

private:
	std::vector<PN_Data> mVertPNData;
	std::vector<TT_Data> mVertTTData;

public:
	KTriMesh();
	void SetupPN(UINT32 pnCnt, bool pnAnim);
	void SetupTT(UINT32 ttCnt, bool ttAnim);

	PN_Data* GetVertPN(UINT32 vidx) {return &mVertPNData[vidx * (mHasPNAnim ? 2 : 1)];}
	const PN_Data* GetVertPN(UINT32 vidx) const {return &mVertPNData[vidx * (mHasPNAnim ? 2 : 1)];}

	TT_Data* GetVertTT(UINT32 vidx) {return &mVertTTData[vidx * (mHasTTAnim ? 2 : 1)];}
	const TT_Data* GetVertTT(UINT32 vidx) const {return &mVertTTData[vidx * (mHasTTAnim ? 2 : 1)];}

	UINT32 FaceCount() const {return (UINT32)mFaces.size();}


	void ChangePivot(const KMatrix4& mat);
	void MakeAsStatic();

	// Following functions will compute the attributes regarding the current time
	void ComputePN_Data(PN_Data& pnData, UINT32 vidx, float cur_t) const;
	void ComputeTT_Data(TT_Data& ttData, UINT32 vidx, float cur_t) const;
	KVec3 ComputeVertPos(UINT32 vidx, float cur_t) const;
	KVec3 ComputeVertNor(UINT32 vidx, float cur_t) const;
	KVec2 ComputeTexcrd(UINT32 vidx, float cur_t) const;
	float ComputeTriArea(UINT32 idx, float cur_t) const;
	void ComputeBBox(KBBox& bbox, float cur_t) const;
	void ComputeFaceNormal(UINT32 idx, KVec3& out_nor, float cur_t) const;
	void InterpolateTT(UINT32 faceIdx, const IntersectContext& ctx, TT_Data& out_tt, float cur_t) const;

	// this function computes the whole bounding box along all the frames
	void ComputeBBoxAll(KBBox& bbox) const;
};


class KScene;
class ISurfaceShader;
class KNode
{
	friend class KScene;
public:
	std::vector<UINT32>	mMesh;
	std::vector<UINT32>	mChild;
	UINT32				mParent;
	ISurfaceShader* mpSurfShader;
	std::string			mName;

private:
	KMatrix4				mNodeTM;	// node transform matrix regarding its parent node
	KMatrix4				mObjectTM;	// node transform matrix in world space
	nvmath::Quatf			mObjectRot;
public:
	KNode();

	const KMatrix4& GetNodeTM() const {return mNodeTM;}
	const KMatrix4& GetObjectTM() const {return mObjectTM;}
	const nvmath::Quatf& GetObjectRot() const {return mObjectRot;}
	bool SaveToFile(FILE* pFile);
	bool LoadFromFile(FILE* pFile);

};


class KScene
{
protected:
	std::vector<KTriMesh*>	mpMesh;
	std::vector<KNode*>		mpNode;

public:
	KScene();
	~KScene();
	void Clean();
	// Functions used to modify the scene
	UINT32 AddMesh();
	UINT32 AddNode();
	KNode* GetNode(UINT32 idx) {return mpNode[idx];}
	KTriMesh* GetMesh(UINT32 idx) {return mpMesh[idx];}
	UINT32 GetTriangleCnt() const;
	const KNode* GetNode(UINT32 idx) const {return mpNode[idx];}
	const KTriMesh* GetMesh(UINT32 idx) const {return mpMesh[idx];}
	UINT32 GetNodeCnt() const {return (UINT32)mpNode.size();}
	UINT32 GetMeshCnt() const {return (UINT32)mpMesh.size();}
	void SetNodeTM(UINT32 nodeIdx, const KMatrix4& tm);
	void GetAccelTriPos(const KTriDesc& tri, KTriVertPos2& triPos, const KBoxNormalizer* pNorm = NULL) const;
	bool IsTriPosAnimated(const KTriDesc& tri) const;
	void GetTriPosData(const KTriDesc& tri, bool anim, float* out_data, const KBoxNormalizer* pNorm = NULL) const;

	// Functions used to calculate accelerated data structure for ray tracing
	void InitAccelTriangleCache(std::vector<KTriDesc>& triCache) const;


	virtual void InitAccelData() {};
	virtual void ResetScene();

private:
	UINT32 GenAccelTriangle(UINT32 nodeIdx, UINT32 subMeshIdx, KTriDesc* accelTri) const;
};

void Vec3TransformCoord(KVec3& res, const KVec3& v, const KMatrix4& mat);
void Vec3TransformCoord(KVec3d& res, const KVec3d& v, const KMatrix4d& mat);
void Vec3TransformCoord(KVec3d& res, const KVec3d& v, const KMatrix4& mat);
void Vec3TransformNormal(KVec3& res, const KVec3& v, const KMatrix4& mat);

bool ClampRayByBBox(KRay& in_out_ray, const KBBox& bbox);


