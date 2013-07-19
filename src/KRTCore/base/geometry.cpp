#include "geometry.h"
#include <string>
#include "../intersection/intersect_ray_tri.h"
#include "../file_io/file_io_template.h"
#include "../material/material_library.h"
#include "../intersection/intersect_ray_bbox.h"
#include <memory.h>
#include <list>
#include <assert.h>

// include the nvmath implementation source code files
#include <common/math/Trafo.h>
#include <common/math/Trafo.cpp>
#include <common/math/Matnnt.cpp>
#include <common/math/Quatt.cpp>

bool ClampRayByBBox(KRay& in_out_ray, const KBBox& bbox);

void KRay::InitInternal(const KVec3& o, const KVec3& d)
{
	*(KVec3*)&mOrign = o;
	*(KVec3*)&mDir = d;

	for (int i = 0; i < 3; ++i) {
		vec4_f(mOrign_shift1, i) = vec4_f(mOrign, (i+1) % 3);
		vec4_f(mDir_shift1, i) = vec4_f(mDir, (i+1) % 3);

		vec4_f(mOrign_shift2, i) = vec4_f(mOrign, (i+2) % 3);
		vec4_f(mDir_shift2, i) = vec4_f(mDir, (i+2) % 3);
	}
	mRcpDir = KVec3(1/d[0], 1/d[1], 1/d[2]);
	mSign[0] = (mRcpDir[0] < 0) ? 1 : 0;
	mSign[1] = (mRcpDir[1] < 0) ? 1 : 0;
	mSign[2] = (mRcpDir[2] < 0) ? 1 : 0;

	vec4_f(mOrign_0011, 0) = o[0];
	vec4_f(mOrign_0011, 1) = o[0];
	vec4_f(mOrign_0011, 2) = o[1];
	vec4_f(mOrign_0011, 3) = o[1];

	vec4_f(mRcpDir_0011, 0) = mRcpDir[0];
	vec4_f(mRcpDir_0011, 1) = mRcpDir[0];
	vec4_f(mRcpDir_0011, 2) = mRcpDir[1];
	vec4_f(mRcpDir_0011, 3) = mRcpDir[1];
	UINT32 shuffle_src0[2] = {0x03020100, 0x07060504};
	UINT32 shuffle_src1[2] = {0x0b0a0908, 0x0f0e0d0c};
	vec4_i(mOrign_shuffle_0011, 0) = shuffle_src0[1 - mSign[0]];
	vec4_i(mOrign_shuffle_0011, 1) = shuffle_src0[mSign[0]];
	vec4_i(mOrign_shuffle_0011, 2) = shuffle_src1[1 - mSign[1]];
	vec4_i(mOrign_shuffle_0011, 3) = shuffle_src1[mSign[1]];

	vec4_f(mOrign_22, 0) = o[2];
	vec4_f(mOrign_22, 1) = o[2];
	vec4_f(mOrign_22, 2) = o[2];
	vec4_f(mOrign_22, 3) = o[2];

	vec4_f(mRcpDir_22, 0) = mRcpDir[2];
	vec4_f(mRcpDir_22, 1) = mRcpDir[2];
	vec4_f(mRcpDir_22, 2) = mRcpDir[2];
	vec4_f(mRcpDir_22, 3) = mRcpDir[2];

	vec4_i(mOrign_shuffle_22, 0) = shuffle_src0[1 - mSign[2]];
	vec4_i(mOrign_shuffle_22, 1) = shuffle_src0[mSign[2]];
	vec4_i(mOrign_shuffle_22, 2) = shuffle_src0[1 - mSign[2]];
	vec4_i(mOrign_shuffle_22, 3) = shuffle_src0[mSign[2]];

	mExcludeBBoxNode = INVALID_INDEX;
	mExcludeTriID = INVALID_INDEX;
}

void KRay::Init(const KVec3& o, const KVec3& d, const KBBox* bbox) 
{
	InitInternal(o, d);
	if (bbox)
		ClampRayByBBox(*this, *bbox);
}

void KRay::InitTranslucentRay(const ShadingContext& shadingCtx, const KBBox* bbox)
{
	Init(shadingCtx.position, shadingCtx.out_vec, bbox);
	mExcludeBBoxNode = shadingCtx.excluding_bbox;
	mExcludeTriID = shadingCtx.excluding_tri;
}

void KRay::InitReflectionRay(const ShadingContext& shadingCtx, const KVec3& in_dir, const KBBox* bbox)
{
	KVec3 reflect = in_dir - shadingCtx.normal * (shadingCtx.normal*in_dir) * 2.0f;
	Init(shadingCtx.position, reflect, bbox);
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

void KScene::InitAccelTriangleCache(std::vector<KAccelTriangle>& triCache) const
{
	// 1.caculate the total triangle count, instanced mesh will be taken into acount
	UINT32 totalTriCnt = 0; 
	for (std::vector<KNode*>::const_iterator it = mpNode.begin(); it != mpNode.end(); ++it) {
		for (std::vector<UINT32>::iterator it_mesh = (*it)->mMesh.begin(); 
			it_mesh != (*it)->mMesh.end(); ++it_mesh) {
			
			totalTriCnt += mpMesh[*it_mesh]->FaceCount();
		}
	}
	// 2. pre-allocate the memory for KAccelTriangle array
	triCache.resize(totalTriCnt);

	// 3. fill the data of each KAccelTriangle objects
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

UINT32 KScene::GenAccelTriangle(UINT32 nodeIdx, UINT32 subMeshIdx, KAccelTriangle* accelTri) const
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

void KScene::GetAccelTriPos(const KAccelTriangle& tri, KAccleTriVertPos& triPos) const
{
	KNode* pNode = mpNode[tri.GetNodeIdx()];
	KTriMesh* pMesh = mpMesh[tri.GetMeshIdx()];
	for (int i_vert = 0; i_vert < 3; ++i_vert) {
		KVec3& pos = pMesh->mVertPN[pMesh->mFaces[tri.mTriIdx].pn_idx[i_vert]].pos;
		KVec4 out_pos;
		out_pos = KVec4(pos, 1.0f) * pNode->GetObjectTM();
		triPos.mVertPos[i_vert][0] = out_pos[0] / out_pos[3];
		triPos.mVertPos[i_vert][1] = out_pos[1] / out_pos[3];
		triPos.mVertPos[i_vert][2] = out_pos[2] / out_pos[3];
				
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


void KTriMesh::InterpolateTT(UINT32 faceIdx, const IntersectContext& ctx, TT_Data& out_tt) const
{
	UINT32 v0 = mTexFaces[faceIdx].tt_idx[0];
	UINT32 v1 = mTexFaces[faceIdx].tt_idx[1];
	UINT32 v2 = mTexFaces[faceIdx].tt_idx[2];
	out_tt.texcoord = mVertTT[v0].texcoord * ctx.w;
	out_tt.texcoord  += mVertTT[v1].texcoord * ctx.u;
	out_tt.texcoord  += mVertTT[v2].texcoord * ctx.v;

	out_tt.tangent = mVertTT[v0].tangent * ctx.w;
	out_tt.tangent  += mVertTT[v1].tangent * ctx.u;
	out_tt.tangent  += mVertTT[v2].tangent * ctx.v;

	out_tt.binormal = mVertTT[v0].binormal * ctx.w;
	out_tt.binormal  += mVertTT[v1].binormal * ctx.u;
	out_tt.binormal  += mVertTT[v2].binormal * ctx.v;
}

void KTriMesh::GetUV(UINT32 faceIdx, const KVec2* uv[3]) const
{
	UINT32 v0 = mTexFaces[faceIdx].tt_idx[0];
	UINT32 v1 = mTexFaces[faceIdx].tt_idx[1];
	UINT32 v2 = mTexFaces[faceIdx].tt_idx[2];
	uv[0] = &mVertTT[v0].texcoord;
	uv[1] = &mVertTT[v1].texcoord;
	uv[2] = &mVertTT[v2].texcoord;
}

UINT32 KScene::GetTriangleCnt() const
{
	UINT32 tri_cnt = 0;
	for (UINT32 i = 0; i < (UINT32)mpMesh.size(); ++i) {
		tri_cnt += (UINT32)mpMesh[i]->FaceCount();
	}
	return tri_cnt;
}

//------------------------- Serializing implementation ------------------------------

bool KTriMesh::SaveToFile(FILE* pFile)
{
	std::string typeName = "KTriMesh";

	if (!SaveArrayToFile(typeName, pFile))
		return false;

	if (!SaveArrayToFile(mFaces, pFile))
		return false;

	if (!SaveArrayToFile(mTexFaces, pFile))
		return false;

	if (!SaveArrayToFile(mVertPN, pFile))
		return false;

	if (!SaveArrayToFile(mVertTT, pFile))
		return false;

	return true;
}

bool KTriMesh::LoadFromFile(FILE* pFile)
{
	std::string srcTypeName;
	std::string dstTypeName = "KTriMesh"; 

	if (!LoadArrayFromFile(srcTypeName, pFile))
		return false;
	if (srcTypeName.compare(dstTypeName) != 0)
		return false;

	if (!LoadArrayFromFile(mFaces, pFile))
		return false;

	if (!LoadArrayFromFile(mTexFaces, pFile))
		return false;

	if (!LoadArrayFromFile(mVertPN, pFile))
		return false;

	if (!LoadArrayFromFile(mVertTT, pFile))
		return false;

	return true;
}

bool KNode::SaveToFile(FILE* pFile)
{
	std::string typeName = "KNode";

	if (!SaveArrayToFile(typeName, pFile))
		return false;

	if (!SaveArrayToFile(mMesh, pFile))
		return false;

	if (!SaveArrayToFile(mChild, pFile))
		return false;

	if (!SaveArrayToFile(mName, pFile))
		return false;

	if (!SaveTypeToFile(mParent, pFile))
		return false;

	if (!SaveTypeToFile(mNodeTM, pFile))
		return false;

	if (!SaveTypeToFile(mObjectTM, pFile))
		return false;

	std::string mtlName = mpSurfShader ? mpSurfShader->GetName() : "";
	if (!SaveArrayToFile(mtlName, pFile))
		return false;

	return true;
}

bool KNode::LoadFromFile(FILE* pFile)
{
	std::string srcTypeName;
	std::string dstTypeName = "KNode"; 

	if (!LoadArrayFromFile(srcTypeName, pFile))
		return false;
	if (srcTypeName.compare(dstTypeName) != 0)
		return false;

	if (!LoadArrayFromFile(mMesh, pFile))
		return false;

	if (!LoadArrayFromFile(mChild, pFile))
		return false;

	if (!LoadArrayFromFile(mName, pFile))
		return false;

	if (!LoadTypeFromFile(mParent, pFile))
		return false;

	if (!LoadTypeFromFile(mNodeTM, pFile))
		return false;

	if (!LoadTypeFromFile(mObjectTM, pFile))
		return false;

	std::string mtlName;
	if (!LoadArrayFromFile(mtlName, pFile))
		return false;
	mpSurfShader = KMaterialLibrary::GetInstance()->OpenMaterial(mtlName.c_str());

	nvmath::Trafo trans_info;
	trans_info.setMatrix(mObjectTM);
	mObjectRot = trans_info.getOrientation();

	return true;
}

bool KScene::SaveToFile(FILE* pFile)
{
	std::string typeName = "KScene";

	if (!SaveArrayToFile(typeName, pFile))
		return false;

	UINT64 meshCnt = mpMesh.size();
	if (!SaveTypeToFile(meshCnt, pFile))
		return false;
	for (UINT32 i = 0; i < meshCnt; ++i) {
		if (!mpMesh[i]->SaveToFile(pFile))
			return false;
	}

	UINT64 nodeCnt = mpNode.size();
	if (!SaveTypeToFile(nodeCnt, pFile))
		return false;
	for (UINT32 i = 0; i < nodeCnt; ++i) {
		if (!mpNode[i]->SaveToFile(pFile))
			return false;
	}

	return true;
}

bool KScene::LoadFromFile(FILE* pFile)
{
	Clean();
	std::string srcTypeName;
	std::string dstTypeName = "KScene"; 

	if (!LoadArrayFromFile(srcTypeName, pFile))
		return false;
	if (srcTypeName.compare(dstTypeName) != 0)
		return false;

	UINT64 meshCnt;
	if (!LoadTypeFromFile(meshCnt, pFile))
		return false;
	for (UINT32 i = 0; i < meshCnt; ++i) {
		KTriMesh* pMesh = new KTriMesh();
		if (!pMesh->LoadFromFile(pFile))
			return false;
		mpMesh.push_back(pMesh);
	}

	UINT64 nodeCnt;
	if (!LoadTypeFromFile(nodeCnt, pFile))
		return false;
	for (UINT32 i = 0; i < nodeCnt; ++i) {
		KNode* pNode = new KNode();
		if (!pNode->LoadFromFile(pFile))
			return false;
		mpNode.push_back(pNode);
	}

	return true;
}


float KTriMesh::ComputeTriArea(UINT32 idx) const
{
	const KVec3& v0 = mVertPN[mFaces[idx].pn_idx[0]].pos;
	const KVec3& v1 = mVertPN[mFaces[idx].pn_idx[1]].pos;
	const KVec3& v2 = mVertPN[mFaces[idx].pn_idx[2]].pos;

	KVec3 edge0 = v1 - v0;
	KVec3 edge1 = v2 - v1;
	// the area is a*b*sin(theta)*0.5
	float dot = edge0 * edge1; // a*b*cos(theta)
	float a2 = nvmath::lengthSquared(edge0); // a^2
	float b2 = nvmath::lengthSquared(edge1); // b^2
	float area2 = a2*b2 - dot*dot;
	return sqrt(area2);
}

void KTriMesh::ComputeFaceNormal(UINT32 idx, KVec3& out_nor) const
{
	const KVec3& v0 = mVertPN[mFaces[idx].pn_idx[0]].pos;
	const KVec3& v1 = mVertPN[mFaces[idx].pn_idx[1]].pos;
	const KVec3& v2 = mVertPN[mFaces[idx].pn_idx[2]].pos;

	KVec3 edge0 = v1 - v0;
	KVec3 edge1 = v2 - v1;
	out_nor = edge0 ^ edge1;
	out_nor.normalize();
}

void KTriMesh::ComputeBBox(KBBox& bbox) const
{
	bbox.SetEmpty();
	for (size_t i = 0; i < mVertPN.size(); ++i)
		bbox.ContainVert(mVertPN[i].pos);
}

void KTriMesh::ChangePivot(const KMatrix4& mat)
{
	KMatrix4 tempMat = mat;
	if (tempMat.invert()) {

		nvmath::Trafo transInfo;
		transInfo.setMatrix(tempMat);
		nvmath::Quatf rot = transInfo.getOrientation();
		for (size_t i = 0; i < mVertPN.size(); ++i) 
			Vec3TransformCoord(mVertPN[i].pos, mVertPN[i].pos, tempMat);
		
		for (size_t i = 0; i < mVertPN.size(); ++i) 
			mVertPN[i].nor = mVertPN[i].nor * rot;
		
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
	float t0, t1;
	if (IntersectBBox(in_out_ray, bbox, t0, t1)) {
		if (t0 < 0) t0 = 0;  // for the case the ray originate from inside the bbox

		KVec3 newPos = in_out_ray.GetOrg() + in_out_ray.GetDir() * t0;
		in_out_ray.InitInternal(newPos, in_out_ray.GetDir());
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
	t = FLT_MAX;
	tri_id = NOT_HIT_INDEX;
	kd_leaf_idx = INVALID_INDEX;
	bbox_node_idx = INVALID_INDEX;

	travel_distance = 0.0f;
}

