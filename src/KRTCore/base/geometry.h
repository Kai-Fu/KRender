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

class KRay 
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

class KTriDesc;
struct KTriVertPos2;
class KBBoxOpt;

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
	void SetEmpty();
	void Enlarge(float factor);

	bool IsEmpty() const;
	bool IsInside(const KVec3& vert) const;
	bool IsOverlapping(const KBSphere& sphere) const;
	const KVec3 Center() const;
	void GetFaceArea(float area[3]) const;
	const KBBox& operator = (const KBBoxOpt& bbox);
};

class KBBoxOpt
{
public:
	vec4 mXXYY;
	vec4 mZZ;

	KBBoxOpt();
	KBBoxOpt(const KBBox& bbox);
	const KBBoxOpt& operator = (const KBBox& bbox);
};

class KBBox4
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

struct KTriVertPos1 {
	KVec3		mVertPos[3];
};

struct KTriVertPos2
{
	KVec3		mVertPos[3];
	KVec3		mVertPos_Ending[3];
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


// This is the optimized version of KTriDesc(four triangles and one ray)
class KAccelTriangleOpt
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

// This is the optimized version of KTriDesc(one triangle and four rays)
class KAccelTriangleOpt1r4t
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

void PrecomputeAccelTri(const KTriVertPos1& tri, UINT32 tri_id, KAccelTriangleOpt &triAccel);

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
	void GetAccelTriPos(const KTriDesc& tri, KTriVertPos2& triPos) const;

	// Functions used to calculate accelerated data structure for ray tracing
	void InitAccelTriangleCache(std::vector<KTriDesc>& triCache) const;


	virtual void InitAccelData() {};
	virtual void ResetScene();

private:
	UINT32 GenAccelTriangle(UINT32 nodeIdx, UINT32 subMeshIdx, KTriDesc* accelTri) const;
};

void Vec3TransformCoord(KVec3& res, const KVec3& v, const KMatrix4& mat);
void Vec3TransformNormal(KVec3& res, const KVec3& v, const KMatrix4& mat);

bool ClampRayByBBox(KRay& in_out_ray, const KBBox& bbox);


