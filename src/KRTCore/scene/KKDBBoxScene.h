#pragma once
#include "../base/geometry.h"
#include "../scene/KKDTreeScene.h"
#include "../animation/animated_transform.h"
#include "../api/KRT_API.h"

class KKDBBoxScene
{
public:
	KKDBBoxScene();
	~KKDBBoxScene();

	void Reset();

	UINT32 SceneNode_Create(UINT32 scene_idx);
	void SceneNodeTM_ResetFrame(UINT32 node_idx);
	void SceneNodeTM_AddFrame(UINT32 node_idx, const KMatrix4& node_tm);
	void SceneNode_ResetAccelData();
	bool SceneNode_BuildAccelData(bool force);
	bool SceneNode_LoadUpdates(FILE* pFile);
	bool SceneNode_SaveUpdates(const std::vector<UINT32>& animated_nodes, const std::vector<UINT32>& animated_scenes, FILE* pFile) const;


	void SetKDSceneCnt(UINT32 cnt);
	UINT32 GetKDSceneCnt() const;
	const KBBox& GetSceneBBox() const;
	KKDTreeScene* GetKDScene(UINT32 idx);
	const KKDTreeScene* GetKDScene(UINT32 idx) const;
	KKDTreeScene* AddKDScene(UINT32& newIdx);

	const KScene* GetNodeKDScene(UINT32 scene_node_idx) const;
	void GetNodeTransform(KAnimation::LocalTRSFrame::LclTRS& out_trs, UINT32 scene_node_idx, float t) const;

	const KAccelTriangle* GetAccelTriData(UINT32 scene_node_idx, UINT32 tri_idx) const;
	float GetSceneEpsilon() const {return mSceneEpsilon;}

	bool IntersectRay_KDTree(const KRay& ray, IntersectContext& ctx, float t) const;

	void GetKDBuildTimeStatistics(KRT_SceneStatistic& sceneStat) const;

	// Save/load the scene and tracking camera data
	bool SaveToFile(FILE* pFile);
	bool LoadFromFile(FILE* pFile);

protected:

	UINT32 SplitBBoxScene(UINT32* pKDSceneIdx, const KBBox* clampBox, UINT32 cnt, UINT32 depth = 0);
	
	bool IntersectBBoxNode(const KRay& ray, UINT32 idx, IntersectContext& ctx, float t) const;
	bool IntersectBBoxLeaf(const KRay& ray, UINT32 idx, IntersectContext& ctx, float t) const;

protected:
	static const UINT32 LEAF_SCENE_CNT = 4;
	struct KD_BBOX_LEAF
	{
		KBBox4 bbox;// this bbox is transformed by the transform matrix of the kd scene
		UINT32 next_leaf_idx;
		UINT32 scene_node_idx[LEAF_SCENE_CNT];
	};
	KVectorA16<KD_BBOX_LEAF> mBBoxLeaf;

	struct KD_BBOX_NODE
	{
		KBBox bbox[2];
		UINT32 child_node[2];
	};
	static const UINT32 LEAF_FLAG = 0x80000000;
	std::vector<KD_BBOX_NODE> mBBoxNode;

	struct KD_SCENE_LEAF
	{
		UINT32 kd_scene_idx;
		KAnimation::LocalTRSFrame scene_trs;
	};
	std::vector<KD_SCENE_LEAF> mKDSceneNodes;
	std::vector<KBBox> mKDSceneBBox;

	std::vector<KKDTreeScene*> mpKDScenes;
	KBBox mSceneBBox;
	float mSceneEpsilon;
	DWORD m_kdBuildTime;
	DWORD m_buildAccelTriTime;
	DWORD m_kdFinializingTime;

	UINT32 mRootNode;

	// temporary data for building accelerating data structure
	std::list<UINT32> mGeomDirtiedScenes;
	bool mIsNodeTMDirty;
	static void TransformRay(KRay& out_ray, const KRay& in_ray, const KD_SCENE_LEAF& scene_info, float t);
};

