#include "KKDBBoxScene.h"
#include "../intersection/intersect_ray_bbox.h"
#include "../file_io/file_io_template.h"
#include <common/math/Trafo.h>
#include "../util/HelperFunc.h"

KAccelTriangleOpt1r4t* KKDTreeScene::s_pGeomBuffer = NULL;
size_t KKDTreeScene::s_geomBufferTriCnt = 0;

KKDBBoxScene::KKDBBoxScene()
{
	mRootNode = INVALID_INDEX;
	KKDTreeScene::s_pGeomBuffer = NULL;

	mIsNodeTMDirty = true;
}

KKDBBoxScene::~KKDBBoxScene()
{
	Reset();
}

void KKDBBoxScene::Reset()
{
	if (KKDTreeScene::s_pGeomBuffer) {
		Aligned_Free(KKDTreeScene::s_pGeomBuffer);
		KKDTreeScene::s_pGeomBuffer = NULL;
	}

	mBBoxNode.clear();
	mBBoxLeaf.clear();
	mKDSceneNodes.clear();
	for (size_t i = 0; i < mpKDScenes.size(); ++i) {
		delete mpKDScenes[i];
	}
	mpKDScenes.clear();
}

void KKDBBoxScene::TransformRay(KRay& out_ray, const KRay& in_ray, const KD_SCENE_LEAF& scene_info, float t)
{
	KAnimation::LocalTRSFrame::LclTRS trs;
	scene_info.scene_trs.Interpolate(t, trs);
	KAnimation::LocalTRSFrame::TransformRay(out_ray, in_ray, trs);
}

UINT32 KKDBBoxScene::SceneNode_Create(UINT32 scene_idx)
{
	KD_SCENE_LEAF leaf;
	leaf.kd_scene_idx = scene_idx;

	mKDSceneNodes.push_back(leaf);
	return UINT32(mKDSceneNodes.size() - 1);
}

void KKDBBoxScene::SceneNodeTM_ResetFrame(UINT32 node_idx)
{
	mKDSceneNodes[node_idx].scene_trs.Reset();
}

void KKDBBoxScene::SceneNodeTM_AddFrame(UINT32 node_idx, const KMatrix4& node_tm)
{
	mKDSceneNodes[node_idx].scene_trs.AddKeyFrame(node_tm);
}


bool KKDBBoxScene::IntersectBBoxNode(const KRay& ray, UINT32 idx, IntersectContext& ctx, float t) const
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
				isHit0 = IntersectBBoxLeaf(ray, child_idx_noMask, ctx, t);
			else
				isHit0 = IntersectBBoxNode(ray, child_idx_noMask, ctx, t);
		}

		if (bHit[1] && ctx.t > t0[1]) {
			UINT32 child_idx = mBBoxNode[idx].child_node[order[1]];
			UINT32 child_idx_noMask = (child_idx & ~LEAF_FLAG);
			float old_t = ctx.t;

			if (child_idx & LEAF_FLAG)
				isHit1 = IntersectBBoxLeaf(ray, child_idx_noMask, ctx, t);
			else
				isHit1 = IntersectBBoxNode(ray, child_idx_noMask, ctx, t);
		}

		res = (isHit0 || isHit1);
	}

	return res;
}

bool KKDBBoxScene::IntersectBBoxLeaf(const KRay& ray, UINT32 idx, IntersectContext& ctx, float t) const
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
			UINT32 scene_idx = mKDSceneNodes[scene_node_idx].kd_scene_idx;

			// transform the ray
			KRay transRay;
			TransformRay(transRay, ray, mKDSceneNodes[scene_node_idx], t);
			if (scene_idx == ray.mExcludeBBoxNode)
				ctx.self_tri_id = ray.mExcludeTriID;
			else
				ctx.self_tri_id = INVALID_INDEX;

			if (mpKDScenes[scene_idx]->IntersectRay_KDTree(transRay, ctx)) {
				ret = true;
				ctx.bbox_node_idx = scene_node_idx;
			}
			
		}
		cur_idx = mBBoxLeaf[cur_idx].next_leaf_idx;
	} while (cur_idx != INVALID_INDEX);

	return ret;
}


bool KKDBBoxScene::IntersectRay_KDTree(const KRay& ray, IntersectContext& ctx, float t) const
{
	UINT32 nearestHitBBoxScene = INVALID_INDEX;
	if (mRootNode != INVALID_INDEX) {
		UINT32 rootNode = (mRootNode & ~LEAF_FLAG);
		if (!mBBoxNode.empty())
			return IntersectBBoxNode(ray, rootNode, ctx, t);
		else
			return IntersectBBoxLeaf(ray, rootNode, ctx, t);
	}
	else
		return false;
}


const KAccelTriangle* KKDBBoxScene::GetAccelTriData(UINT32 scene_node_idx, UINT32 tri_idx) const
{
	UINT32 scene_idx = mKDSceneNodes[scene_node_idx].kd_scene_idx;
	return mpKDScenes[scene_idx]->GetAccelTriData(tri_idx);
}


UINT32 KKDBBoxScene::SplitBBoxScene(UINT32* pKDSceneIdx, const KBBox* clampBox, UINT32 cnt, UINT32 depth)
{
	KBBox bbox;
	UINT32 ret = INVALID_INDEX;

	if (NULL == pKDSceneIdx) {
		for (size_t i = 0; i < mKDSceneNodes.size(); ++i) {
			bbox.Add(mKDSceneBBox[i]);
		}
		cnt = (UINT32)mKDSceneNodes.size();
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

void KKDBBoxScene::GetKDBuildTimeStatistics(SceneStatistic& sceneStat) const
{
	sceneStat.kd_build_time = m_kdBuildTime;
	sceneStat.gen_accel_geom_time = m_buildAccelTriTime;
	sceneStat.kd_finialize_time = m_kdFinializingTime;

	sceneStat.bbox_leaf_count = (UINT32)mKDSceneNodes.size();
	sceneStat.bbox_node_count = (UINT32)mBBoxNode.size();

	sceneStat.kd_node_count = 0;
	sceneStat.kd_leaf_count = 0;
	sceneStat.actual_triangle_count = 0;
	sceneStat.leaf_triangle_count = 0;
	for (size_t i = 0; i < mpKDScenes.size(); ++i) {
		KKDTreeScene* pScene = mpKDScenes[mKDSceneNodes[i].kd_scene_idx];
		sceneStat.leaf_triangle_count += pScene->mTotalLeafTriCnt;
		sceneStat.kd_node_count += pScene->GetKDNodeCnt();
		sceneStat.kd_leaf_count += pScene->GetKDLeafCnt();
		sceneStat.actual_triangle_count += pScene->GetTriangleCnt();
	}
}


bool KKDBBoxScene::SceneNode_BuildAccelData(bool force)
{
	mSceneBBox.SetEmpty();
	mSceneEpsilon = FLT_MAX;
	m_kdBuildTime = 0;
	m_buildAccelTriTime = 0;
	m_kdFinializingTime = 0;

	if (force) {
		// if it's forced to update, all the accellerating structures will be re-computed.
		for (UINT32 i = 0; i < mpKDScenes.size(); ++i)
			mGeomDirtiedScenes.push_back(i);
	}

	// Now build all KD scenes
	for (std::list<UINT32>::iterator it = mGeomDirtiedScenes.begin(); it != mGeomDirtiedScenes.end(); ++it) {
		// build the accellerating data strcuture for each KD scene
		mpKDScenes[*it]->InitAccelData();

		// update the scene epsilon(which is be used later for ray intersection)
		float cur_epsilon = mpKDScenes[mKDSceneNodes[*it].kd_scene_idx]->GetSceneEpsilon();
		if (mSceneEpsilon > cur_epsilon)
			mSceneEpsilon = cur_epsilon;

		// keep track of the time spent on building accellerating data
		DWORD kd_build, gen_accel;
		mpKDScenes[*it]->GetKDBuildTimeStatistics(kd_build, gen_accel);
		m_kdBuildTime += kd_build;
		m_buildAccelTriTime += gen_accel;
	}

	// Now compute the scene epsilon
	if (mIsNodeTMDirty || !mGeomDirtiedScenes.empty()) {
		mSceneEpsilon = FLT_MAX;
		mKDSceneBBox.resize(mKDSceneNodes.size());
		for (size_t i = 0; i < mKDSceneBBox.size(); ++i) {
			mKDSceneNodes[i].scene_trs.ComputeTotalBBox(mpKDScenes[mKDSceneNodes[i].kd_scene_idx]->GetSceneBBox(), mKDSceneBBox[i]);
			mSceneBBox.Add(mKDSceneBBox[i]);
			float cur_epsilon = mpKDScenes[mKDSceneNodes[i].kd_scene_idx]->GetSceneEpsilon();
			if (mSceneEpsilon > cur_epsilon)
				mSceneEpsilon = cur_epsilon;
		}

		mRootNode = SplitBBoxScene(NULL, NULL, 0);
		mKDSceneBBox.clear();
	}

	// Now finalize all the accellerating data structure by copying it into the final buffer.
	if (!mGeomDirtiedScenes.empty()) {
		// the final buffer only needs to be updated when the geometry data of any KD scene is changed.
		KTimer stop_watch(true);
		std::vector<size_t> nodeGeomBufSize;
		size_t totalGeomBufferSize = 0;
		nodeGeomBufSize.resize(mpKDScenes.size());
		for (size_t i = 0; i < mpKDScenes.size(); ++i) {
			nodeGeomBufSize[i] = mpKDScenes[i]->CalcGeomDataSize();
			totalGeomBufferSize += nodeGeomBufSize[i];
		}

		if (totalGeomBufferSize > 0) {
			KKDTreeScene::s_pGeomBuffer = 
				(KAccelTriangleOpt1r4t*)Aligned_Malloc(totalGeomBufferSize * sizeof(KAccelTriangleOpt1r4t), 16);
			if (!KKDTreeScene::s_pGeomBuffer) {
				printf("Out of memory, exit now...\n");
				exit(0);
			}
			KKDTreeScene::s_geomBufferTriCnt = totalGeomBufferSize;
		}
		size_t fill_offset = 0;
		for (size_t i = 0; i < mpKDScenes.size(); ++i) {
			mpKDScenes[i]->FinalizeKDTree(fill_offset);
			fill_offset += nodeGeomBufSize[i];
		}

		double time_elapsed = stop_watch.Stop();
		m_kdFinializingTime = DWORD(time_elapsed * 1000.0);
	}

	return true;
}



const KScene* KKDBBoxScene::GetNodeKDScene(UINT32 scene_node_idx) const
{
	UINT32 scene_idx = mKDSceneNodes[scene_node_idx].kd_scene_idx;
	return  mpKDScenes[scene_idx]; 
}

void KKDBBoxScene::GetNodeTransform(KAnimation::LocalTRSFrame::LclTRS& out_trs, UINT32 scene_node_idx, float t) const
{
	mKDSceneNodes[scene_node_idx].scene_trs.Interpolate(t, out_trs);
}

void KKDBBoxScene::SetKDSceneCnt(UINT32 cnt)
{
	Reset();
	mpKDScenes.resize(cnt);
	for (size_t i = 0; i < mpKDScenes.size(); ++i) {
		mpKDScenes[i] = new KKDTreeScene();
	}
}

UINT32 KKDBBoxScene::GetKDSceneCnt() const
{
	return (UINT32)mpKDScenes.size();
}

KKDTreeScene* KKDBBoxScene::GetKDScene(UINT32 idx)
{
	return  mpKDScenes[idx];
}

const KKDTreeScene* KKDBBoxScene::GetKDScene(UINT32 idx) const
{
	return  mpKDScenes[idx];
}

KKDTreeScene* KKDBBoxScene::AddKDScene(UINT32& newIdx)
{
	mpKDScenes.resize(mpKDScenes.size() + 1);
	newIdx = (UINT32)mpKDScenes.size() - 1;
	mpKDScenes[newIdx] = new KKDTreeScene();
	return mpKDScenes[newIdx];
}

const KBBox& KKDBBoxScene::GetSceneBBox() const
{
	return mSceneBBox;
}

void KKDBBoxScene::SceneNode_ResetAccelData()
{
	mIsNodeTMDirty = true;
}

bool KKDBBoxScene::SceneNode_LoadUpdates(FILE* pFile)
{
	std::string srcTypeName;
	std::string dstTypeName = "Scene updates"; 

	if (!LoadArrayFromFile(srcTypeName, pFile))
		return false;
	if (srcTypeName.compare(dstTypeName) != 0)
		return false;

	mGeomDirtiedScenes.clear();

	std::vector<UINT32> animated_nodes;
	if (!LoadArrayFromFile(animated_nodes, pFile))
		return false;
	for (size_t i = 0; i < animated_nodes.size(); ++i) {
		UINT32 idx = animated_nodes[i];
		if (!LoadTypeFromFile(mKDSceneNodes[idx].kd_scene_idx, pFile))
			return false;
		if (!mKDSceneNodes[idx].scene_trs.Load(pFile))
			return false;
	}

	std::vector<UINT32> animated_scenes;
	if (!LoadArrayFromFile(animated_scenes, pFile))
		return false;
	for (size_t i = 0; i < animated_scenes.size(); ++i) {
		UINT32 idx = animated_scenes[i];
		if (!mpKDScenes[mKDSceneNodes[idx].kd_scene_idx]->LoadFromFile(pFile))
			return false;
		mGeomDirtiedScenes.push_back(idx);
	}

	SceneNode_BuildAccelData(false);
	return true;
}

bool KKDBBoxScene::SceneNode_SaveUpdates(const std::vector<UINT32>& animated_nodes, const std::vector<UINT32>& animated_scenes, FILE* pFile) const
{
	std::string typeName = "Scene updates";

	if (!SaveArrayToFile(typeName, pFile))
		return false;

	if (!SaveArrayToFile(animated_nodes, pFile))
		return false;
	for (std::vector<UINT32>::const_iterator it = animated_nodes.begin(); it != animated_nodes.end(); ++it) {
		if (!SaveTypeToFile(mKDSceneNodes[*it].kd_scene_idx, pFile))
			return false;
		if (!mKDSceneNodes[*it].scene_trs.Save(pFile))
			return false;
	}

	if (!SaveArrayToFile(animated_scenes, pFile))
		return false;
	for (std::vector<UINT32>::const_iterator it = animated_scenes.begin(); it != animated_scenes.end(); ++it) {
		if (!mpKDScenes[mKDSceneNodes[*it].kd_scene_idx]->SaveToFile(pFile))
			return false;
	}

	return true;
}

bool KKDBBoxScene::SaveToFile(FILE* pFile)
{
	std::string typeName = "KKDBBoxScene";

	if (!SaveArrayToFile(typeName, pFile))
		return false;

	UINT64 sceneCnt = mpKDScenes.size();
	if (!SaveTypeToFile(sceneCnt, pFile))
		return false;
	for (UINT32 i = 0; i < sceneCnt; ++i) {
		if (!mpKDScenes[i]->SaveToFile(pFile))
			return false;
	}

	UINT64 nodeCnt = mKDSceneNodes.size();
	if (!SaveTypeToFile(nodeCnt, pFile))
		return false;
	for (UINT32 i = 0; i < nodeCnt; ++i) {
		if (!SaveTypeToFile(mKDSceneNodes[i].kd_scene_idx, pFile))
			return false;
		if (!mKDSceneNodes[i].scene_trs.Save(pFile))
			return false;
	}

	return true;
}

bool KKDBBoxScene::LoadFromFile(FILE* pFile)
{
	Reset();
	std::string srcTypeName;
	std::string dstTypeName = "KKDBBoxScene"; 

	if (!LoadArrayFromFile(srcTypeName, pFile))
		return false;
	if (srcTypeName.compare(dstTypeName) != 0)
		return false;

	UINT64 sceneCnt;
	if (!LoadTypeFromFile(sceneCnt, pFile))
		return false;
	mpKDScenes.resize((size_t)sceneCnt);

	for (UINT32 i = 0; i < sceneCnt; ++i) {
		mpKDScenes[i] = new KKDTreeScene;
		if (!mpKDScenes[i]->LoadFromFile(pFile))
			return false;
	}
	
	UINT64 nodeCnt;
	if (!LoadTypeFromFile(nodeCnt, pFile))
		return false;
	mKDSceneNodes.resize((size_t)nodeCnt);
	for (UINT32 i = 0; i < nodeCnt; ++i) {
		if (!LoadTypeFromFile(mKDSceneNodes[i].kd_scene_idx, pFile))
			return false;
		if (!mKDSceneNodes[i].scene_trs.Load(pFile))
			return false;
	}

	return true;
}

