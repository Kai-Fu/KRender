#include "KKDBBoxScene.h"
#include "../intersection/intersect_ray_bbox.h"
#include "../file_io/file_io_template.h"
#include <common/math/Trafo.h>
#include "../util/HelperFunc.h"


KAccelStruct_BVH::KAccelStruct_BVH(const KSceneSet* sceneSet)
{
	mRootNode = INVALID_INDEX;

	mpSceneSet = NULL;
	mpAccelGeomBuffer = NULL;
	mpSceneSet = sceneSet;
}

KAccelStruct_BVH::~KAccelStruct_BVH()
{
	if (mpAccelGeomBuffer) {
		Aligned_Free(mpAccelGeomBuffer);
		mpAccelGeomBuffer = NULL;
	}

}

const KSceneSet* KAccelStruct_BVH::GetSource() const
{
	return mpSceneSet;
}

KSceneSet::KSceneSet()
{

}

KSceneSet::~KSceneSet()
{

}

void KSceneSet::Reset()
{
	mKDSceneNodes.clear();
	for (size_t i = 0; i < mpKDScenes.size(); ++i) {
		delete mpKDScenes[i];
	}
	mpKDScenes.clear();
}

void KAccelStruct_BVH::TransformRay(KRay& out_ray, const KRay& in_ray, const KSceneSet::KD_SCENE_LEAF& scene_info, float t)
{
	KAnimation::LocalTRSFrame::LclTRS trs;
	scene_info.scene_trs.Interpolate(t, trs);
	KAnimation::LocalTRSFrame::TransformRay(out_ray, in_ray, trs);
}

UINT32 KSceneSet::SceneNode_Create(UINT32 scene_idx)
{
	KD_SCENE_LEAF leaf;
	leaf.kd_scene_idx = scene_idx;

	mKDSceneNodes.push_back(leaf);
	return UINT32(mKDSceneNodes.size() - 1);
}

void KSceneSet::SceneNodeTM_SetStaticNode(UINT32 node_idx, const KMatrix4& trans)
{
	mKDSceneNodes[node_idx].scene_trs.Reset(trans);
}

void KSceneSet::SceneNodeTM_SetMovingNode(UINT32 node_idx, const KMatrix4& starting, const KMatrix4& ending)
{
	mKDSceneNodes[node_idx].scene_trs.Reset(starting, ending);
}

bool KAccelStruct_BVH::IntersectBBoxNode(const KRay& ray, UINT32 idx, IntersectContext& ctx, float cur_t) const
{
	bool res = false;
	if (!mBBoxNode.empty()) {
		float t0[2] = {FLT_MAX,FLT_MAX};
		float t1[2] = {FLT_MAX,FLT_MAX};
		bool bHit[2];
		UINT32 order[2] = {0, 1};
		bHit[0] = IntersectBBox(ray, mBBoxNode[idx].bbox[0], t0[0], t1[0]);
		bHit[1] = IntersectBBox(ray, mBBoxNode[idx].bbox[1], t0[1], t1[1]);
		if (t0[0] > t0[1]) {
			std::swap(t0[0], t0[1]);
			std::swap(t1[0], t1[1]);
			std::swap(bHit[0], bHit[1]);
			std::swap(order[0], order[1]);
		}

		bool isHit0 = false;
		bool isHit1 = false;

		if (bHit[0]) {
			UINT32 child_idx = mBBoxNode[idx].child_node[order[0]];
			UINT32 child_idx_noMask = (child_idx & ~LEAF_FLAG);
			float old_t = ctx.t;

			if (child_idx & LEAF_FLAG)
				isHit0 = IntersectBBoxLeaf(ray, child_idx_noMask, ctx, cur_t);
			else
				isHit0 = IntersectBBoxNode(ray, child_idx_noMask, ctx, cur_t);
		}

		if (bHit[1] && ctx.t > t0[1]) {
			UINT32 child_idx = mBBoxNode[idx].child_node[order[1]];
			UINT32 child_idx_noMask = (child_idx & ~LEAF_FLAG);
			float old_t = ctx.t;

			if (child_idx & LEAF_FLAG)
				isHit1 = IntersectBBoxLeaf(ray, child_idx_noMask, ctx, cur_t);
			else
				isHit1 = IntersectBBoxNode(ray, child_idx_noMask, ctx, cur_t);
		}

		res = (isHit0 || isHit1);
	}

	return res;
}

bool KAccelStruct_BVH::IntersectBBoxLeaf(const KRay& ray, UINT32 idx, IntersectContext& ctx, float cur_t) const
{
	UINT32 cur_idx = idx;
	vec4f t0;
	vec4f t1;
	bool ret = false;

	do {
		int scene_cnt = 0;
		int hit_scene[LEAF_SCENE_CNT];

		vec4i hit_res = IntersectBBox(ray, mBBoxLeaf[cur_idx].bbox, t0, t1);
		float* pT0;
		for (UINT32 i = 0; i < 4; ++i) {
			if (vec4_i(hit_res, i)) {
				pT0 = (float*)&t0 + i;
				break;
			}
		}

		for (UINT32 i = 0; i < LEAF_SCENE_CNT; ++i) {
			if (mBBoxLeaf[cur_idx].scene_node_idx[i] == INVALID_INDEX)
				break;
			
			if (vec4_i(hit_res, i)) {
				hit_scene[scene_cnt] = i;
				++scene_cnt;
			}

		}
		
		// Here I sort the order the the sub-scenes to be tested, doing so could potentially improve
		// the performance because the nearer sub-scene gets tested first.
		int scene_order[LEAF_SCENE_CNT] = {0,1,2,3};
		for (int i = 0; i < scene_cnt; ++i) {
			for (int j = scene_cnt - 1; j > i; --j) {
				if (vec4_f(t0, scene_order[j-1]) > vec4_f(t0, scene_order[j]))
					std::swap(scene_order[j-1], scene_order[j]);
			}
		}
		
		for (int i = 0; i < scene_cnt; ++i) {
			UINT32 scene_node_idx = mBBoxLeaf[cur_idx].scene_node_idx[hit_scene[scene_order[i]]];
			UINT32 scene_idx = mpSceneSet->GetNodeSceneIndex(scene_node_idx);

			// transform the ray
			KRay transRay;
			TransformRay(transRay, ray, mpSceneSet->mKDSceneNodes[scene_node_idx], cur_t);
			if (scene_idx == ray.mExcludeBBoxNode)
				ctx.self_tri_id = ray.mExcludeTriID;
			else
				ctx.self_tri_id = INVALID_INDEX;

			if (mpAccelStructs[scene_idx]->IntersectRay_KDTree(transRay, cur_t, ctx)) {
				ret = true;
				ctx.bbox_node_idx = scene_node_idx;
			}
			
		}
		cur_idx = mBBoxLeaf[cur_idx].next_leaf_idx;
	} while (cur_idx != INVALID_INDEX);

	return ret;
}


bool KAccelStruct_BVH::IntersectRay_KDTree(const KRay& ray, float cur_t, IntersectContext& ctx) const
{
	UINT32 nearestHitBBoxScene = INVALID_INDEX;
	if (mRootNode != INVALID_INDEX) {
		UINT32 rootNode = (mRootNode & ~LEAF_FLAG);
		if (!mBBoxNode.empty())
			return IntersectBBoxNode(ray, rootNode, ctx, cur_t);
		else
			return IntersectBBoxLeaf(ray, rootNode, ctx, cur_t);
	}
	else
		return false;
}


const KTriDesc* KAccelStruct_BVH::GetAccelTriData(UINT32 scene_node_idx, UINT32 tri_idx) const
{
	UINT32 scene_idx = mpSceneSet->mKDSceneNodes[scene_node_idx].kd_scene_idx;
	return mpAccelStructs[scene_idx]->GetAccelTriData(tri_idx);
}


UINT32 KAccelStruct_BVH::SplitBBoxScene(UINT32* pKDSceneIdx, const KBBox* clampBox, UINT32 cnt, UINT32 depth)
{
	KBBox bbox;
	UINT32 ret = INVALID_INDEX;

	if (NULL == pKDSceneIdx) {
		for (size_t i = 0; i < mpAccelStructs.size(); ++i) {
			bbox.Add(mKDSceneBBox[i]);
		}
		cnt = (UINT32)mpAccelStructs.size();
	}
	else {
		for (size_t i = 0; i < cnt; ++i) {
			UINT32 idx = pKDSceneIdx[i];
			bbox.Add(mKDSceneBBox[idx]);
		}
	}
	if (clampBox) {
		bbox.ClampBBox(*clampBox);
	}
	bool bAsLeaf = true;
	if (cnt > LEAF_SCENE_CNT && depth < 20) {


		KVec3 det = bbox.mMax - bbox.mMin;
		float det_value;
		UINT32 det_axis;
		KD_BBOX_NODE newBBoxNode;
		newBBoxNode.bbox[0] = bbox;
		newBBoxNode.bbox[1] = bbox;
		if (fabs(det[0]) >= fabs(det[1]) && fabs(det[0]) >= fabs(det[2])) {
			// Split X axis
			det_axis = 0;
		}
		else if (fabs(det[1]) >= fabs(det[0]) && fabs(det[1]) >= fabs(det[2])) {
			// Split Y axis
			det_axis = 1;
		}
		else {
			// Split Z axis
			det_axis = 2;
		}
		det_value = (bbox.mMax[det_axis] + bbox.mMin[det_axis]) * 0.5f;
		newBBoxNode.bbox[0].mMin[det_axis] = det_value;
		newBBoxNode.bbox[1].mMax[det_axis] = det_value;

		// Split the bbox scene
		UINT32* pLeftChild = (UINT32*)malloc(sizeof(UINT32)*cnt);
		UINT32* pRightChild = (UINT32*)malloc(sizeof(UINT32)*cnt);
		UINT32 cnt0 = 0;
		UINT32 cnt1 = 0;
		KBBox left_bbox;
		KBBox right_bbox;
		UINT32 stride_cnt = 0; // triangle count that stride over two sides
		for (UINT32 i = 0; i < cnt; ++i) {
			UINT32 idx = (pKDSceneIdx ? pKDSceneIdx[i] : i);
			if (mKDSceneBBox[idx].mMin[det_axis] > det_value) {
				// this triangle is in + side
				pLeftChild[cnt0++] = idx;
				left_bbox.Add(mKDSceneBBox[idx]);
			}
			else if (mKDSceneBBox[idx].mMax[det_axis] < det_value) {
				// this triangle is in - side
				pRightChild[cnt1++] = idx;
				right_bbox.Add(mKDSceneBBox[idx]);
			}
			else {
				// this triangle stride over two sides
				pLeftChild[cnt0++] = idx;
				pRightChild[cnt1++] = idx;
				left_bbox.Add(mKDSceneBBox[idx]);
				right_bbox.Add(mKDSceneBBox[idx]);
				++stride_cnt;
			}
		}

		if (stride_cnt < (cnt/2)) {
			left_bbox.ClampBBox(newBBoxNode.bbox[0]);
			right_bbox.ClampBBox(newBBoxNode.bbox[1]);
			newBBoxNode.bbox[0] = left_bbox;
			newBBoxNode.bbox[1] = right_bbox;
			newBBoxNode.child_node[0] = SplitBBoxScene(pLeftChild, &left_bbox, cnt0, depth+1);
			newBBoxNode.child_node[1] = SplitBBoxScene(pRightChild, &right_bbox, cnt1, depth+1);

			mBBoxNode.push_back(newBBoxNode);
			ret = UINT32(mBBoxNode.size() - 1);
			bAsLeaf = false;
		}
		else {
			free(pLeftChild);
			free(pRightChild);
		}

	}

	if (cnt > 0 && bAsLeaf) {
		KD_BBOX_LEAF bbox_leaf;
		UINT32 last_idx = INVALID_INDEX;

		UINT32 offset = 0;
		do {
			UINT32 cur_cnt = ((cnt - offset) > LEAF_SCENE_CNT) ? LEAF_SCENE_CNT : (cnt - offset);
			KBBox tempBox[4];
			for (UINT32 i = 0; i < cur_cnt; ++i) {
				UINT32 scene_node_idx = pKDSceneIdx ? pKDSceneIdx[offset+i] : (offset+i);
				tempBox[i] = mKDSceneBBox[scene_node_idx];
				bbox_leaf.scene_node_idx[i] = scene_node_idx;
			}
			bbox_leaf.bbox.FromBBox(tempBox, cur_cnt);

			for (UINT32 i = cur_cnt; i < LEAF_SCENE_CNT; ++i) {
				bbox_leaf.scene_node_idx[i] = INVALID_INDEX;
			}
			offset += cur_cnt;
			bbox_leaf.next_leaf_idx = last_idx;
			last_idx = (UINT32)mBBoxLeaf.size();
			mBBoxLeaf.Append(bbox_leaf);
		} while (cnt > offset);

		ret = UINT32(mBBoxLeaf.size() - 1);
		ret |= LEAF_FLAG;
	}

	if (pKDSceneIdx)
		free(pKDSceneIdx);

	return ret;
}

void KAccelStruct_BVH::GetKDBuildTimeStatistics(KRT_SceneStatistic& sceneStat) const
{
	sceneStat.kd_build_time = m_kdBuildTime;
	sceneStat.gen_accel_geom_time = m_buildAccelTriTime;
	sceneStat.kd_finialize_time = m_kdFinializingTime;

	sceneStat.bbox_leaf_count = (UINT32)mpAccelStructs.size();
	sceneStat.bbox_node_count = (UINT32)mBBoxNode.size();

	sceneStat.kd_node_count = 0;
	sceneStat.kd_leaf_count = 0;
	sceneStat.actual_triangle_count = 0;
	sceneStat.leaf_triangle_count = 0;
	for (size_t i = 0; i < mpAccelStructs.size(); ++i) {
		KAccelStruct* pAccel = mpAccelStructs[i];
		KScene* pScene = mpSceneSet->mpKDScenes[i];
		sceneStat.leaf_triangle_count += pAccel->GetAccelLeafTriCnt();
		sceneStat.kd_node_count += pAccel->GetAccelNodeCnt();
		sceneStat.kd_leaf_count += pAccel->GetAccelLeafCnt();
		sceneStat.actual_triangle_count += pScene->GetTriangleCnt();
	}
}


bool KAccelStruct_BVH::SceneNode_BuildAccelData(const std::list<UINT32>* pDirtiedSubScene)
{
	mSceneBBox.SetEmpty();
	mSceneEpsilon = FLT_MAX;
	m_kdBuildTime = 0;
	m_buildAccelTriTime = 0;
	m_kdFinializingTime = 0;

	std::list<UINT32> tempDirtiedSubScene;
	if (pDirtiedSubScene == NULL) {
		// if it's forced to update, all the accellerating structures will be re-computed.
		for (size_t i = 0; i < mpAccelStructs.size(); ++i) {
			delete mpAccelStructs[i];
		}
		mpAccelStructs.resize(mpSceneSet->mpKDScenes.size());
		for (UINT32 i = 0; i < mpSceneSet->mpKDScenes.size(); ++i) {
			KAccelStruct_KDTree* pAccel = new KAccelStruct_KDTree(mpSceneSet->mpKDScenes[i]);
			mpAccelStructs[i] = pAccel;
			tempDirtiedSubScene.push_back(i);
		}
	}
	else
		tempDirtiedSubScene = *pDirtiedSubScene;

	// Now build all KD scenes
	for (std::list<UINT32>::iterator it = tempDirtiedSubScene.begin(); it != tempDirtiedSubScene.end(); ++it) {

		// build the accellerating data strcuture for each KD scene
		mpAccelStructs[*it]->InitAccelData();

		// update the scene epsilon(which is be used later for ray intersection)
		float cur_epsilon = mpAccelStructs[*it]->GetSceneEpsilon();
		if (mSceneEpsilon > cur_epsilon)
			mSceneEpsilon = cur_epsilon;

		// keep track of the time spent on building accellerating data
		DWORD kd_build, gen_accel;
		mpAccelStructs[*it]->GetKDBuildTimeStatistics(kd_build, gen_accel);
		m_kdBuildTime += kd_build;
		m_buildAccelTriTime += gen_accel;
	}

	// Now compute the scene epsilon
	{
		mSceneEpsilon = FLT_MAX;
		mKDSceneBBox.resize(mpSceneSet->mKDSceneNodes.size());
		for (size_t i = 0; i < mKDSceneBBox.size(); ++i) {
			mpSceneSet->mKDSceneNodes[i].scene_trs.ComputeTotalBBox(mpAccelStructs[mpSceneSet->mKDSceneNodes[i].kd_scene_idx]->GetSceneBBox(), mKDSceneBBox[i]);
			mSceneBBox.Add(mKDSceneBBox[i]);
			float cur_epsilon = mpAccelStructs[mpSceneSet->mKDSceneNodes[i].kd_scene_idx]->GetSceneEpsilon();
			if (mSceneEpsilon > cur_epsilon)
				mSceneEpsilon = cur_epsilon;
		}

		mRootNode = SplitBBoxScene(NULL, NULL, 0);
		mKDSceneBBox.clear();
	}

	// Now finalize all the accellerating data structure by copying it into the final buffer.
	{
		// the final buffer only needs to be updated when the geometry data of any KD scene is changed.
		KTimer stop_watch(true);
		std::vector<size_t> nodeGeomBufSize;
		size_t totalGeomBufferSize = 0;
		nodeGeomBufSize.resize(mpSceneSet->mpKDScenes.size());
		for (size_t i = 0; i < mpSceneSet->mpKDScenes.size(); ++i) {
			nodeGeomBufSize[i] = mpAccelStructs[i]->CalcGeomDataSize();
			totalGeomBufferSize += nodeGeomBufSize[i];
		}

		if (totalGeomBufferSize > 0) {
			mpAccelGeomBuffer = 
				(KAccelTriangleOpt1r4t*)Aligned_Malloc(totalGeomBufferSize * sizeof(KAccelTriangleOpt1r4t), 16);
			if (!mpAccelGeomBuffer) {
				printf("Out of memory, exit now...\n");
				exit(0);
			}
		}
		size_t fill_offset = 0;
		for (size_t i = 0; i < mpAccelStructs.size(); ++i) {
			mpAccelStructs[i]->FinalizeKDTree(mpAccelGeomBuffer + fill_offset);
			fill_offset += nodeGeomBufSize[i];
		}

		double time_elapsed = stop_watch.Stop();
		m_kdFinializingTime = DWORD(time_elapsed * 1000.0);
	}

	return true;
}

const KScene* KSceneSet::GetNodeKDScene(UINT32 scene_node_idx) const
{
	UINT32 scene_idx = mKDSceneNodes[scene_node_idx].kd_scene_idx;
	return  mpKDScenes[scene_idx]; 
}

UINT32 KSceneSet::GetNodeSceneIndex(UINT32 scene_node_idx) const
{
	UINT32 scene_idx = mKDSceneNodes[scene_node_idx].kd_scene_idx;
	return scene_idx;
}

void KSceneSet::GetNodeTransform(KAnimation::LocalTRSFrame::LclTRS& out_trs, UINT32 scene_node_idx, float t) const
{
	mKDSceneNodes[scene_node_idx].scene_trs.Interpolate(t, out_trs);
}

void KSceneSet::SetKDSceneCnt(UINT32 cnt)
{
	Reset();
	mpKDScenes.resize(cnt);
	for (size_t i = 0; i < mpKDScenes.size(); ++i) {
		mpKDScenes[i] = new KScene();
	}
}

UINT32 KSceneSet::GetKDSceneCnt() const
{
	return (UINT32)mpKDScenes.size();
}

KScene* KSceneSet::GetKDScene(UINT32 idx)
{
	return  mpKDScenes[idx];
}

const KScene* KSceneSet::GetKDScene(UINT32 idx) const
{
	return  mpKDScenes[idx];
}

KScene* KSceneSet::AddKDScene(UINT32& newIdx)
{
	mpKDScenes.resize(mpKDScenes.size() + 1);
	newIdx = (UINT32)mpKDScenes.size() - 1;
	mpKDScenes[newIdx] = new KScene();
	return mpKDScenes[newIdx];
}

const KBBox& KAccelStruct_BVH::GetSceneBBox() const
{
	return mSceneBBox;
}


