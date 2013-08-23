#include "geometry.h"
#include <string>
#include "../intersection/intersect_ray_tri.h"
#include "../file_io/file_io_template.h"
#include "../material/material_library.h"
#include "../intersection/intersect_ray_bbox.h"
#include "../util/HelperFunc.h"
#include <memory.h>
#include <list>
#include <assert.h>

// include the nvmath implementation source code files
#include <common/math/Trafo.h>
#include <common/math/Trafo.cpp>
#include <common/math/Matnnt.cpp>
#include <common/math/Quatt.cpp>

bool ClampRayByBBox(KRay& in_out_ray, const KBBox& bbox);

KVec3d ToVec3d(const KVec3& ref)
{
	return KVec3d(ref[0], ref[1], ref[2]);
}

KVec3 ToVec3f(const KVec3d& ref)
{
	return KVec3(float(ref[0]), float(ref[1]), float(ref[2]));
}

KVec4d ToVec4d(const KVec4& ref)
{
	return KVec4d(ref[0], ref[1], ref[2], ref[3]);
}

KMatrix4d ToMatrix4d(const KMatrix4& ref)
{
	KMatrix4d ret;
	ret[0] = ToVec4d(ref[0]);
	ret[1] = ToVec4d(ref[1]);
	ret[2] = ToVec4d(ref[2]);
	ret[3] = ToVec4d(ref[3]);
	return ret;
}

void KRay::Init(const KVec3d& o, const KVec3d& d)
{
	mOrign = o;
	mDir = d;

	mRcpDir = KVec3d(1/d[0], 1/d[1], 1/d[2]);
	mSign[0] = (mRcpDir[0] < 0) ? 1 : 0;
	mSign[1] = (mRcpDir[1] < 0) ? 1 : 0;
	mSign[2] = (mRcpDir[2] < 0) ? 1 : 0;

	mExcludeBBoxNode = INVALID_INDEX;
	mExcludeTriID = INVALID_INDEX;
}

void KRay::Init(const KVec3d& o, const KVec3d& d, const KBBox* clamp_bbox) 
{
	Init(o, d);
	if (clamp_bbox)
		ClampRayByBBox(*this, *clamp_bbox);
}

void KRay::InitTranslucentRay(const ShadingContext& shadingCtx, const KBBox* bbox)
{
	Init(ToVec3d(shadingCtx.position), ToVec3d(shadingCtx.out_vec), bbox);
	mExcludeBBoxNode = shadingCtx.excluding_bbox;
	mExcludeTriID = shadingCtx.excluding_tri;
}

void KRay::InitReflectionRay(const ShadingContext& shadingCtx, const KVec3& in_dir, const KBBox* bbox)
{
	KVec3 reflect = in_dir - shadingCtx.normal * (shadingCtx.normal*in_dir) * 2.0f;
	Init(ToVec3d(shadingCtx.position), ToVec3d(reflect), bbox);
	mExcludeBBoxNode = shadingCtx.excluding_bbox;
	mExcludeTriID = shadingCtx.excluding_tri;
}

KNode::KNode() 
{
	mParent = INVALID_INDEX;
}

void KScene::SetNodeTM(UINT32 nodeIdx, const KMatrix4& tm)
{
	KNode* pNode = mpNode[nodeIdx];
	pNode->mNodeTM = tm;
	if (INVALID_INDEX == pNode->mParent) {
		pNode->mObjectTM = tm;
	}
	else {
		pNode->mObjectTM = mpNode[pNode->mParent]->GetObjectTM() * tm;
	}
	nvmath::Trafo trans_info;
	trans_info.setMatrix(pNode->mObjectTM);
	pNode->mObjectRot = trans_info.getOrientation();

	for (UINT32 i_node = 0; i_node < pNode->mChild.size(); ++i_node) {
		KNode* pChildNode = mpNode[pNode->mChild[i_node]];
		SetNodeTM(pNode->mChild[i_node], pChildNode->GetNodeTM());
	}
}

void KScene::InitAccelTriangleCache(std::vector<KTriDesc>& triCache) const
{
	// 1.caculate the total triangle count, instanced mesh will be taken into acount
	UINT32 totalTriCnt = 0; 
	for (std::vector<KNode*>::const_iterator it = mpNode.begin(); it != mpNode.end(); ++it) {
		for (std::vector<UINT32>::iterator it_mesh = (*it)->mMesh.begin(); 
			it_mesh != (*it)->mMesh.end(); ++it_mesh) {
			
			totalTriCnt += mpMesh[*it_mesh]->FaceCount();
		}
	}
	// 2. pre-allocate the memory for KTriDesc array
	triCache.resize(totalTriCnt);

	// 3. fill the data of each KTriDesc objects
	UINT32 fillPos = 0;
	for (UINT32 i_node = 0; i_node < mpNode.size(); ++i_node) {
		for (std::vector<UINT32>::iterator it_mesh = mpNode[i_node]->mMesh.begin(); 
			it_mesh != mpNode[i_node]->mMesh.end(); ++it_mesh) {
			if (mpMesh[*it_mesh]->FaceCount() == 0)
				continue;
			fillPos += GenAccelTriangle(i_node, *it_mesh, &triCache[fillPos]);
		}
	}
}

UINT32 KScene::GenAccelTriangle(UINT32 nodeIdx, UINT32 subMeshIdx, KTriDesc* accelTri) const
{
	KNode* pNode = mpNode[nodeIdx];
	KTriMesh* pMesh = mpMesh[subMeshIdx];
	UINT32 faceCnt = pMesh->FaceCount();
	if (accelTri) {
		for (UINT32 i = 0; i < faceCnt; ++i) {

			accelTri[i].SetNodeMeshIdx(nodeIdx, subMeshIdx);
			accelTri[i].mTriIdx = i;
		}
	}
	return faceCnt;
}

void KScene::GetAccelTriPos(const KTriDesc& tri, KTriVertPos2& triPos, const KBoxNormalizer* pNorm) const
{
	KNode* pNode = mpNode[tri.GetNodeIdx()];
	KTriMesh* pMesh = mpMesh[tri.GetMeshIdx()];
	KMatrix4 mat = pNode->GetObjectTM();
	if (pNorm)
		pNorm->ApplyToMatrix(mat);
	triPos.mIsMoving = pMesh->mHasPNAnim;

	for (int i_vert = 0; i_vert < 3; ++i_vert) {

		const KTriMesh::PN_Data* pPN_Data = pMesh->GetVertPN(pMesh->mFaces[tri.mTriIdx].pn_idx[i_vert]);

		const KVec3& pos = pPN_Data->pos;
		KVec4 out_pos;
		out_pos = KVec4(pos, 1.0f) * mat;
		triPos.mVertPos[i_vert][0] = out_pos[0] / out_pos[3];
		triPos.mVertPos[i_vert][1] = out_pos[1] / out_pos[3];
		triPos.mVertPos[i_vert][2] = out_pos[2] / out_pos[3];

		if (triPos.mIsMoving) {
			++pPN_Data;

			const KVec3& pos = pPN_Data->pos;
			KVec4 out_pos;
			out_pos = KVec4(pos, 1.0f) * mat;
			triPos.mVertPos_Ending[i_vert][0] = out_pos[0] / out_pos[3];
			triPos.mVertPos_Ending[i_vert][1] = out_pos[1] / out_pos[3];
			triPos.mVertPos_Ending[i_vert][2] = out_pos[2] / out_pos[3];
		}
				
	}
}

UINT32 KScene::AddMesh()
{
	mpMesh.push_back(NULL);
	UINT32 lastIdx = (UINT32)mpMesh.size() - 1;
	mpMesh[lastIdx] = new KTriMesh;
	return lastIdx;
}

UINT32 KScene::AddNode()
{
	mpNode.push_back(NULL);
	UINT32 lastIdx = (UINT32)mpNode.size() - 1;
	mpNode[lastIdx] = new KNode;
	return lastIdx;
}


KScene::KScene()
{

}

KScene::~KScene()
{
	Clean();
}

void KScene::Clean()
{
	for (UINT32 i = 0; i < mpNode.size(); ++i) {
		delete mpNode[i];
	}
	mpNode.clear();

	for (UINT32 i = 0; i < mpMesh.size(); ++i) {
		delete mpMesh[i];
	}
	mpMesh.clear();
}

void KScene::ResetScene()
{
	Clean();
}


void KTriMesh::InterpolateTT(UINT32 faceIdx, const IntersectContext& ctx, TT_Data& out_tt, float cur_t) const
{
	UINT32 v0 = mTexFaces[faceIdx].tt_idx[0];
	UINT32 v1 = mTexFaces[faceIdx].tt_idx[1];
	UINT32 v2 = mTexFaces[faceIdx].tt_idx[2];

	TT_Data tt0, tt1, tt2;
	ComputeTT_Data(tt0, v0, cur_t);
	ComputeTT_Data(tt1, v1, cur_t);
	ComputeTT_Data(tt2, v2, cur_t);
	out_tt.texcoord = tt0.texcoord * ctx.w;
	out_tt.texcoord  += tt1.texcoord * ctx.u;
	out_tt.texcoord  += tt2.texcoord * ctx.v;

	out_tt.tangent = tt0.tangent * ctx.w;
	out_tt.tangent  += tt1.tangent * ctx.u;
	out_tt.tangent  += tt2.tangent * ctx.v;

	out_tt.binormal = tt0.binormal * ctx.w;
	out_tt.binormal  += tt1.binormal * ctx.u;
	out_tt.binormal  += tt2.binormal * ctx.v;
}

void KTriMesh::MakeAsStatic()
{
	for (size_t i = 0; i < mVertPNData.size(); ++i) {
		mVertPNData[i] = mVertPNData[i*2];
	}
	mVertPNData.resize(mVertPNData.size() / 2);

	for (size_t i = 0; i < mVertPNData.size(); ++i) {
		mVertTTData[i] = mVertTTData[i*2];
	}
	mVertTTData.resize(mVertTTData.size() / 2);
	mHasPNAnim = false;
}

UINT32 KScene::GetTriangleCnt() const
{
	UINT32 tri_cnt = 0;
	for (UINT32 i = 0; i < (UINT32)mpMesh.size(); ++i) {
		tri_cnt += (UINT32)mpMesh[i]->FaceCount();
	}
	return tri_cnt;
}

KTriMesh::KTriMesh()
{
	mHasPNAnim = false;
	mPN_VertCnt = 0;

	mHasTTAnim = false;
	mTT_VertCnt = 0;
}

void KTriMesh::SetupPN(UINT32 pnCnt, bool pnAnim)
{
	mVertPNData.resize(pnCnt * (pnAnim ? 2 : 1));
	mHasPNAnim = pnAnim;
	mPN_VertCnt = pnCnt;
	
}

void KTriMesh::SetupTT(UINT32 ttCnt, bool ttAnim)
{
	mVertTTData.resize(ttCnt * (ttAnim ? 2 : 1));
	mHasTTAnim = ttAnim;
	mTT_VertCnt = ttCnt;
}

void KTriMesh::ComputePN_Data(PN_Data& pnData, UINT32 vidx, float cur_t) const
{
	if (mHasPNAnim) {
		float alpha = cur_t;
		const PN_Data* pVert = GetVertPN(vidx);
		pnData.pos = nvmath::lerp(alpha, pVert[0].pos, pVert[1].pos);
		pnData.nor = nvmath::lerp(alpha, pVert[0].nor, pVert[1].nor);
	}
	else
		pnData = mVertPNData[vidx];
}

void KTriMesh::ComputeTT_Data(TT_Data& ttData, UINT32 vidx, float cur_t) const
{
	if (mHasTTAnim) {
		float alpha = cur_t;
		const TT_Data* pVert = GetVertTT(vidx);
		ttData.texcoord = nvmath::lerp(alpha, pVert[0].texcoord, pVert[1].texcoord);
		ttData.tangent = nvmath::lerp(alpha, pVert[0].tangent, pVert[1].tangent);
		ttData.binormal = nvmath::lerp(alpha, pVert[0].binormal, pVert[1].binormal);
	}
	else
		ttData = mVertTTData[vidx];
}

KVec3 KTriMesh::ComputeVertPos(UINT32 vidx, float cur_t) const
{
	if (mHasPNAnim) {
		float alpha = cur_t;
		const PN_Data* pVert = GetVertPN(vidx);
		return nvmath::lerp(alpha, pVert[0].pos, pVert[1].pos);
	}
	else
		return mVertPNData[vidx].pos;
}

KVec3 KTriMesh::ComputeVertNor(UINT32 vidx, float cur_t) const
{
	if (mHasPNAnim) {
		float alpha = cur_t;
		const PN_Data* pVert = GetVertPN(vidx);
		return nvmath::lerp(alpha, pVert[0].nor, pVert[1].nor);
	}
	else
		return mVertPNData[vidx].nor;
}

KVec2 KTriMesh::ComputeTexcrd(UINT32 vidx, float cur_t) const
{
	if (mHasTTAnim) {
		float alpha = cur_t;
		const TT_Data* pVert = GetVertTT(vidx);
		return nvmath::lerp(alpha, pVert[0].texcoord, pVert[1].texcoord);
	}
	else
		return mVertTTData[vidx].texcoord;
}

float KTriMesh::ComputeTriArea(UINT32 idx, float cur_t) const
{
	const KVec3& v0 = ComputeVertPos(mFaces[idx].pn_idx[0], cur_t);
	const KVec3& v1 = ComputeVertPos(mFaces[idx].pn_idx[1], cur_t);
	const KVec3& v2 = ComputeVertPos(mFaces[idx].pn_idx[2], cur_t);

	KVec3 edge0 = v1 - v0;
	KVec3 edge1 = v2 - v1;
	// the area is a*b*sin(theta)*0.5
	float dot = edge0 * edge1; // a*b*cos(theta)
	float a2 = nvmath::lengthSquared(edge0); // a^2
	float b2 = nvmath::lengthSquared(edge1); // b^2
	float area2 = a2*b2 - dot*dot;
	return sqrt(area2);
}

void KTriMesh::ComputeFaceNormal(UINT32 idx, KVec3& out_nor, float cur_t) const
{
	const KVec3& v0 = ComputeVertPos(mFaces[idx].pn_idx[0], cur_t);
	const KVec3& v1 = ComputeVertPos(mFaces[idx].pn_idx[1], cur_t);
	const KVec3& v2 = ComputeVertPos(mFaces[idx].pn_idx[2], cur_t);

	KVec3 edge0 = v1 - v0;
	KVec3 edge1 = v2 - v1;
	out_nor = edge0 ^ edge1;
	out_nor.normalize();
}

void KTriMesh::ComputeBBoxAll(KBBox& bbox) const
{
	bbox.SetEmpty();
	for (size_t i = 0; i < mVertPNData.size(); ++i)
		bbox.ContainVert(mVertPNData[i].pos);
}

void KTriMesh::ComputeBBox(KBBox& bbox, float cur_t) const
{
	bbox.SetEmpty();
	for (UINT32 i = 0; i < mPN_VertCnt; ++i)
		bbox.ContainVert(ComputeVertPos(i, cur_t));
}

void KTriMesh::ChangePivot(const KMatrix4& mat)
{
	KMatrix4 tempMat = mat;
	if (tempMat.invert()) {

		nvmath::Trafo transInfo;
		transInfo.setMatrix(tempMat);
		nvmath::Quatf rot = transInfo.getOrientation();
		for (size_t i = 0; i < mVertPNData.size(); ++i) 
			Vec3TransformCoord(mVertPNData[i].pos, mVertPNData[i].pos, tempMat);
		
		for (size_t i = 0; i < mVertPNData.size(); ++i) 
			mVertPNData[i].nor = mVertPNData[i].nor * rot;
		
	}

}



struct tTangentItem
{
	UINT32 vertIdx;
	UINT32 normIdx;
	TangentData tangent;
	UINT32 offset;
};

class tTangentSet : public std::vector<tTangentItem>
{
public:
	tTangentSet() {reserve(3);}
};

// Clamp the input ray against the bounding box the scene, most of the time 
// this can improve the floating point precision.
bool ClampRayByBBox(KRay& in_out_ray, const KBBox& bbox)
{
	double t0, t1;
	if (IntersectBBox(in_out_ray, bbox, t0, t1)) {
		if (t0 < 0) t0 = 0;  // for the case the ray originate from inside the bbox

		KVec3d newPos = in_out_ray.GetOrg() + in_out_ray.GetDir() * t0;
		in_out_ray.Init(newPos, in_out_ray.GetDir());
		return true;
	}
	else 
		return false;
}

IntersectContext::IntersectContext() 
{
	Reset();
}

void IntersectContext::Reset()
{
	ray_t = FLT_MAX;
	tri_id = NOT_HIT_INDEX;
	kd_leaf_idx = INVALID_INDEX;
	bbox_node_idx = INVALID_INDEX;
}

