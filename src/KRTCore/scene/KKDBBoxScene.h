#pragma once
#include "../base/geometry.h"
#include "../scene/KKDTreeScene.h"
#include "../animation/animated_transform.h"
#include "../api/KRT_API.h"

class KSceneSet
{
public:
	KSceneSet();
	~KSceneSet();

	void Reset();

	UINT32 SceneNode_Create(UINT32 scene_idx);
	void SceneNodeTM_SetStaticNode(UINT32 node_idx, const KMatrix4& trans);
	void SceneNodeTM_SetMovingNode(UINT32 node_idx, const KMatrix4& starting, const KMatrix4& ending);

	void SetKDSceneCnt(UINT32 cnt);
	UINT32 GetKDSceneCnt() const;
	KScene* GetKDScene(UINT32 idx);
	const KScene* GetKDScene(UINT32 idx) const;
	KScene* AddKDScene(UINT32& newIdx);

	const KScene* GetNodeKDScene(UINT32 scene_node_idx) const;
	UINT32 GetNodeSceneIndex(UINT32 scene_node_idx) const;
	void GetNodeTransform(KAnimation::LocalTRSFrame::LclTRS& out_trs, UINT32 scene_node_idx, float t) const;

public:
	struct KD_SCENE_LEAF
	{
		UINT32 kd_scene_idx;
		KAnimation::LocalTRSFrame scene_trs;
	};

	std::vector<KScene*> mpKDScenes;
	std::vector<KD_SCENE_LEAF> mKDSceneNodes;
};


class KAccelStruct_BVH
{
public:
	KAccelStruct_BVH(const KSceneSet* sceneSet);
	~KAccelStruct_BVH();

	const KSceneSet* GetSource() const;

	void SceneNode_ResetAccelData();
	bool SceneNode_BuildAccelData(bool force);
	bool SceneNode_LoadUpdates(FILE* pFile);
	bool SceneNode_SaveUpdates(const std::vector<UINT32>& animated_nodes, const std::vector<UINT32>& animated_scenes, FILE* pFile) const;

	const KTriDesc* GetAccelTriData(UINT32 scene_node_idx, UINT32 tri_idx) const;
	float GetSceneEpsilon() const {return mSceneEpsilon;}
	const KBBox& GetSceneBBox() const;

	bool IntersectRay_KDTree(const KRay& ray, float cur_t, IntersectContext& ctx) const;

	void GetKDBuildTimeStatistics(KRT_SceneStatistic& sceneStat) const;

	// Save/load the scene and tracking camera data
	bool SaveToFile(FILE* pFile);
	bool LoadFromFile(FILE* pFile);

protected:

	UINT32 SplitBBoxScene(UINT32* pKDSceneIdx, const KBBox* clampBox, UINT32 cnt, UINT32 depth = 0);
	
	bool IntersectBBoxNode(const KRay& ray, UINT32 idx, IntersectContext& ctx, float cur_t) const;
	bool IntersectBBoxLeaf(const KRay& ray, UINT32 idx, IntersectContext& ctx, float cur_t) const;

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

	const KSceneSet* mpSceneSet;
	std::vector<KAccelStruct*> mpAccelStructs;
	KAccelTriangleOpt1r4t* mpAccelGeomBuffer;

	std::vector<KD_BBOX_NODE> mBBoxNode;
	std::vector<KBBox> mKDSceneBBox;
	
	KBBox mSceneBBox;
	float mSceneEpsilon;
	DWORD m_kdBuildTime;
	DWORD m_buildAccelTriTime;
	DWORD m_kdFinializingTime;

	UINT32 mRootNode;

	// temporary data for building accelerating data structure
	std::list<UINT32> mGeomDirtiedScenes;
	bool mIsNodeTMDirty;
	static void TransformRay(KRay& out_ray, const KRay& in_ray, const KSceneSet::KD_SCENE_LEAF& scene_info, float t);
};

