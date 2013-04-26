#pragma once
#include "BaseHeader.h"
#include "../image/color.h"
#include <vector>
#include <string>
#include <xmmintrin.h>
#ifdef _WIN32
	#include <tmmintrin.h>
#else

#endif

// Force to align in 16 byte to improve cache coherency
#define INVALID_INDEX 0xffffffff
#define NOT_HIT_INDEX 0xfffffffe



typedef __m128 vec4f;
typedef __m128i vec4i;
union vec4
{
	vec4f asFloat4;
	vec4i asUINT4;
};
#define vec4_i(vec, idx) (((UINT32*)&vec)[idx])
#define vec4_f(vec, idx) (((float*)&vec)[idx])


#define vec4_madd(a, b, c) (_mm_add_ps(_mm_mul_ps((a), (b)), (c)))
#define vec4_add(a, b)    _mm_add_ps((a), (b))
#define vec4_sub(a, b)    _mm_sub_ps((a), (b))
#define vec4_div(a, b)    _mm_div_ps((a), (b))
#define vec4_mul(a, b)    _mm_mul_ps((a), (b))
#define vec4_splats(a)    _mm_set_ps1((a))
#define vec4_rcp(a)        _mm_rcp_ps((a))
#define vec4_and(a, b)    _mm_and_ps((a), (b))
#define vec4_cmpgt(a, b)  _mm_cmpgt_ps((a), (b))
#define vec4_cmpge(a, b)  _mm_cmpge_ps((a), (b))
#define vec4_cmple(a, b)  _mm_cmple_ps((a), (b))
#define vec4_cmplt(a, b)  _mm_cmplt_ps((a), (b))
#define vec4_cmpgt(a, b)  _mm_cmpgt_ps((a), (b))
#define vec4_gather(a)    _mm_movemask_ps((a))
#define vec4_zero()       _mm_setzero_ps()
#define vec4_st(a, b)     _mm_store_ps((a), (b))
#define vec4_sel(a, b, c) _mm_or_ps(_mm_and_ps((b), (c)), _mm_andnot_ps((c), (a)))

struct ShadingContext;
class KBBox;

struct IntersectContext
{
	float t;
	float w, u, v;
	UINT32 tri_id;
	UINT32 kd_leaf_idx;
	UINT32 bbox_node_idx;
	float travel_distance;

	UINT32 self_tri_id;

	KVec3 walkVec;
	IntersectContext();
	void Reset();
};

class KRT_API KRay 
{
private:
	vec4 mOrign;
	vec4 mDir;
public:

	KVec3 mRcpDir;
	int mSign[4];

	vec4 mOrign_shift1;
	vec4 mDir_shift1;
	vec4 mOrign_shift2;
	vec4 mDir_shift2;

	vec4f mOrign_0011;
	vec4f mRcpDir_0011;
	vec4i mOrign_shuffle_0011;
	vec4f mOrign_22;
	vec4f mRcpDir_22;
	vec4i mOrign_shuffle_22;

	// triangle ID used to exclude the hit testing
	UINT32 mExcludeBBoxNode;
	UINT32 mExcludeTriID;

public:
	// this method should not be used explicitly
	void InitInternal(const KVec3& o, const KVec3& d);

	// methods that are real public:)
	void Init(const KVec3& o, const KVec3& d, const KBBox* bbox);
	void InitTranslucentRay(const ShadingContext& shadingCtx, const KBBox* bbox);
	void InitReflectionRay(const ShadingContext& shadingCtx, const KVec3& in_dir, const KBBox* bbox);
	
	const KVec3& GetOrg() const {return *(KVec3*)&mOrign;}
	const KVec3& GetDir() const {return *(KVec3*)&mDir;}

	const vec4& GetOrgVec4() const {return mOrign;}
	const vec4& GetDirVec4() const {return mDir;}
};

class KAccelTriangle;
struct KAccleTriVertPos;
class KBBoxOpt;

class KRT_API KBSphere
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


class KRT_API KBBox
{
public:
	KVec3 mMin;
	KVec3 mMax;

	KBBox();
	KBBox(const KAccleTriVertPos& tri);
	const KVec3& operator[](int i) const {return i ? mMax : mMin;}
	void Add(const KBBox& bbox);
	void ContainVert(const KVec3& vert);
	void ClampBBox(const KBBox& bbox);
	void TransformByMatrix(const KMatrix4& mat);
	void SetEmpty();
	void Enlarge(float factor);

	bool IsEmpty() const;
	bool IsInside(const KVec3& vert) const;
	bool IsOverlapping(const KBSphere& sphere) const;
	const KVec3 Center() const;
	void GetFaceArea(float area[3]) const;
	const KBBox& operator = (const KBBoxOpt& bbox);
};

class KRT_API KBBoxOpt
{
public:
	vec4 mXXYY;
	vec4 mZZ;

	KBBoxOpt();
	KBBoxOpt(const KBBox& bbox);
	const KBBoxOpt& operator = (const KBBox& bbox);
};

class KRT_API KBBox4
{
public:
	vec4f mMin[3];
	vec4f mMax[3];

	const vec4f* operator[](int i) const {return i ? mMax : mMin;}
	void FromBBox(const KBBox* bbox, UINT32 cnt);
};

struct KTriangle {
	UINT32 pn_idx[3]; // position & normal index
};

struct KTexTriangle {
	UINT32 tt_idx[3];  // texcoord & tangent index
};

struct KAccleTriVertPos {
	KVec3		mVertPos[3];
};

// Data structure to accelerate ray-triangle intersection
class KRT_API KAccelTriangle
{
public:
	UINT32	mNodeMeshIdx;
	UINT32	mTriIdx;

	UINT32 GetMeshIdx() const {return (mNodeMeshIdx & 0x0000ffff);}
	UINT32 GetNodeIdx() const {return ((mNodeMeshIdx & 0xffff0000) >> 16);}

	void SetNodeMeshIdx(UINT32 nodeIdx, UINT32 meshIdx) {mNodeMeshIdx = ((nodeIdx << 16) | (meshIdx & 0x0000ffff));}
};


// This is the optimized version of KAccelTriangle(four triangles and one ray)
class KRT_API KAccelTriangleOpt
{
public:
	// first 16 byte half cache line
	// plane:
	float n_u;	// normal.u / normal.k
	float n_v;	// normal.v / normal.k
	float n_d;	// constant of plane equation
	UINT32 k;		// projection dimension

	// second 16 byte half cache line
	float b_nu;
	float b_nv;
	float b_d;
	UINT32 tri_id;	// triangle id, it's also for pad to next cache line(triangle index)

	// third of 16 byte half cache line
	// line equation for line ab
	float c_nu;
	float c_nv;
	float c_d;
	float E_F;	// data use to store the normal flag(sign bit) and the uv epsilon value
};

// This is the optimized version of KAccelTriangle(one triangle and four rays)
class KRT_API KAccelTriangleOpt1r4t
{
public:
	// first 16 byte half cache line
	// plane:
	vec4f n_u;	// normal.u / normal.k
	vec4f n_v;	// normal.v / normal.k
	vec4f n_d;	// constant of plane equation
	vec4i k;		// projection dimension

	// second 16 byte half cache line
	vec4f b_nu;
	vec4f b_nv;
	vec4f b_d;
	vec4f tri_id;	// triangle id, it's also for pad to next cache line(triangle index)

	// third of 16 byte half cache line
	// line equation for line ab
	vec4f c_nu;
	vec4f c_nv;
	vec4f c_d;
	vec4f E_F;	// max edge length, it's also for pad to next cache line(triangle index)

};

void KRT_API PrecomputeAccelTri(const KAccleTriVertPos& tri, UINT32 tri_id, KAccelTriangleOpt &triAccel);

class KRT_API KTriMesh
{

public:
	std::vector<KTriangle> mFaces;
	std::vector<KTexTriangle> mTexFaces;
	struct PN_Data {
		KVec3 pos;
		KVec3 nor;
	};
	std::vector<PN_Data> mVertPN;

	struct TT_Data {
		KVec2 texcoord;
		KVec3 tangent;
		KVec3 binormal;
	};
	std::vector<TT_Data> mVertTT;
public:

	UINT32 FaceCount() {return (UINT32)mFaces.size();}
	UINT32 PNCount() {return (UINT32)mVertPN.size();}

	float ComputeTriArea(UINT32 idx) const;
	void ComputeBBox(KBBox& bbox) const;
	void ChangePivot(const KMatrix4& mat);

	void ComputeFaceNormal(UINT32 idx, KVec3& out_nor) const;

	void InterpolateTT(UINT32 faceIdx, const IntersectContext& ctx, TT_Data& out_tt) const;
	void GetUV(UINT32 faceIdx, const KVec2* uv[3]) const;

	bool SaveToFile(FILE* pFile);
	bool LoadFromFile(FILE* pFile);

};


class KScene;
class ISurfaceShader;
class KRT_API KNode
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


class KRT_API KScene
{
protected:
	std::vector<KTriMesh*>	mpMesh;
	std::vector<KNode*>		mpNode;
	std::vector<KAccelTriangle>	mAccelTriangle;

	float				mSceneEpsilon;

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
	const KAccelTriangle* GetAccelTriData(UINT32 idx) const {return &mAccelTriangle[idx];}
	float GetSceneEpsilon() const {return mSceneEpsilon;}
	void GetAccelTriPos(const KAccelTriangle& tri, KAccleTriVertPos& triPos) const;

	// Functions used to calculate accelerated data structure for ray tracing
	void InitAccelTriangleCache();

	// Functions to do the ray tracing
	bool IntersectRay_BruteForce(const KRay& ray, IntersectContext& ctx);

	virtual void InitAccelData() {};
	virtual void ResetScene();

	bool SaveToFile(FILE* pFile);
	bool LoadFromFile(FILE* pFile);

private:
	UINT32 GenAccelTriangle(UINT32 nodeIdx, UINT32 subMeshIdx, KAccelTriangle* accelTri);
};

void Vec3TransformCoord(KVec3& res, const KVec3& v, const KMatrix4& mat);
void Vec3TransformNormal(KVec3& res, const KVec3& v, const KMatrix4& mat);

bool KRT_API ClampRayByBBox(KRay& in_out_ray, const KBBox& bbox);


