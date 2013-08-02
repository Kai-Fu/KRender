#pragma once
#include "../util/thread_model.h"
#include "../base/geometry.h"
#include <vector>
#include <set>
#include "../os/api_wrapper.h"
#include "../base/kvector.h"
#include "../util/memory_pool.h"
#include <memory>

class KAccelStruct_KDTree;

class SceneSplitTask : public ThreadModel::IThreadTask 
{
public:
	KAccelStruct_KDTree* pKDScene;
	UINT32* triangles;
	UINT32 cnt;
	UINT32 split_flag;
	KBBox clamp_box;
	UINT32 depth;
	int needBBox;
	UINT32 destIndex;
	UINT32 splitThreadIdx;

	virtual void Execute();
};

class KAccelStruct
{
public:
	virtual bool IntersectRay_KDTree(const KRay& ray, IntersectContext& ctx) const {return false;}
	virtual bool IntersectRay_BruteForce(const KRay& ray, IntersectContext& ctx) const {return false;}
	virtual unsigned long long GetAccelLeafTriCnt() const = 0;
	virtual unsigned long long GetAccelNodeCnt() const = 0;
	virtual unsigned long long GetAccelLeafCnt() const = 0;
	virtual void InitAccelData() = 0;
	virtual float GetSceneEpsilon() const = 0;
	virtual void GetKDBuildTimeStatistics(DWORD& kd_build, DWORD& gen_accel) const = 0;
	virtual const KBBox& GetSceneBBox() const = 0;
	virtual size_t CalcGeomDataSize() const = 0;
	virtual void FinalizeKDTree(KAccelTriangleOpt1r4t* pGeomBuffer) = 0;

	const KTriDesc* GetAccelTriData(UINT32 tri_idx) const;
protected:
	std::vector<KTriDesc>	mAccelTriangle;
};

class KAccelStruct_KDTree : public KAccelStruct
{
public:
	KAccelStruct_KDTree(const KScene* scene);
	virtual ~KAccelStruct_KDTree();

	const KScene* GetSource() const;
public:
	friend class SceneSplitTask;
	typedef struct _KD_LeafData {
		
		union TRI_LIST {
			KAccelTriangleOpt1r4t* accel_tri4;
			UINT32* leaf_triangles;
		} tri_list;
		KBBox bbox;
		UINT32  tri_cnt;		
	} KD_LeafData;

	enum NodeFlag{
		eSplitX = 0x00,
		eSplitY = 0x01,
		eSplitZ = 0x02,

		eLeftChild		= 0x0010,
		eRightChild		= 0x0020,
		ePerfectSplit	= 0x0040,
		eNoBBox			= 0x0080,

		eSplitAxisMask = 0x000f
	};
	// Data structure for each kd node
	struct KD_Node {
		long flag;
		UINT32  left_child;
		UINT32  right_child;
		float  split_value;
		KBBoxOpt bbox;
	};
	struct KD_Node_NoBBox {
		long flag;
		UINT32  left_child;
		UINT32  right_child;
		float  split_value;
	};

	// Statistic info of this kd tree	
	UINT32 mTotalLeafTriCnt;
	UINT32 mPerfectsplitCnt;	// splitting the doesn't intersect any triangle
	KBBox mSceneBBox;
	DWORD m_kdBuildTime;
	DWORD m_buildAccelTriTime;

protected:
	
	// All the kd node data is stored here
	const KScene* mpSourceScene;
	float mSceneEpsilon;

	KVectorA16<KD_Node_NoBBox>	mSceneNode;
	std::vector<KD_LeafData> mKDLeafData;
	GlowableMemPool mLeafIdxMemPool;

	// Limitations when build the kd tree
	UINT32	mMaxDepth;
	float	mNodeSplitThreshhold;
	UINT32	mLeafTriCnt;
	
	UINT32 mRootNode;	// root node's index
	UINT32 mProcessorCnt;
	
	struct SplitData 
	{
		UINT32 in_splitThreadIdx;
	};

	class DATA_FOR_KD_BUILD 
	{
	public:
		volatile long mActiveSplitThreadCnt;
		std::auto_ptr<ThreadModel::ThreadBucket> mThreadBucket;
		std::auto_ptr<ThreadModel::ThreadPool> mThreadPool;
		std::vector<SceneSplitTask> mSplitTasks;
		std::vector<KBBox> mTriBBox;
		struct TRI_IDX_ARRAY {
			UINT32 tri_cnt;
			UINT32* pData;

			TRI_IDX_ARRAY() {tri_cnt = 0; pData = NULL;}
			~TRI_IDX_ARRAY() {free(pData);}
			void Resize(UINT32 cnt) {
				if (cnt > tri_cnt) {
					pData = (UINT32*)realloc(pData, cnt*sizeof(UINT32));
					tri_cnt = cnt;
				}
			}
		};
		typedef std::vector<TRI_IDX_ARRAY> LEVELS_OF_TRI_ARRAY;
		std::vector<LEVELS_OF_TRI_ARRAY> mSplitThreadTriArrayLevel;
		std::vector<std::vector<UINT32> > mPigeonHoles;
		
		SPIN_LOCK_FLAG		mNodeAddingCS; 
		SPIN_LOCK_FLAG		mLeafAddingCS; 

		int			mThreadLimit;
		UINT32		mCPUCnt;

		DATA_FOR_KD_BUILD();
		~DATA_FOR_KD_BUILD();
		
		void NotifySplitThreadAdded();
		void NotifySplitThreadComplete(UINT32 thread_idx);
		UINT32* AcquireTempTriIdxBuf(UINT32 threadIdx, int depth, UINT32 cnt);
	};
	std::auto_ptr<DATA_FOR_KD_BUILD> mTempDataForKD;

protected:	
	UINT32 BuildKDNode4Data(UINT32 idx);
	bool IntersectNode(UINT32 idx, const KRay& ray, IntersectContext& ctx) const;
	bool IntersectLeaf(UINT32 idx, const KRay& ray, IntersectContext& ctx) const;
	int PrepareKDTree();
	void FillAccelTri1r4t(const std::vector<KAccelTriangleOpt>& kuv_tri, KAccelTriangleOpt1r4t* acc_tri, UINT32 offset_tri4, bool pad_end) const;
	void PrecomputeTriangleBBox();

public:
	UINT32 SplitScene(UINT32* triangles, UINT32 cnt, 
		int needBBox,
		const KBBox* clamp_box, 
		const SplitData& splitData,
		int depth, bool& isLeaf);
	virtual void InitAccelData();
	virtual void ResetScene();

	bool IntersectRay_KDTree(const KRay& ray, IntersectContext& ctx) const;
	bool IntersectRay_BruteForce(const KRay& ray, IntersectContext& ctx) const;

	const KTriDesc* GetAccelTriData(UINT32 tri_idx) const {return &mAccelTriangle[tri_idx];} 

	virtual float GetSceneEpsilon() const {return mSceneEpsilon;}
	virtual unsigned long long GetAccelLeafTriCnt() const {return mTotalLeafTriCnt;}
	virtual unsigned long long GetAccelNodeCnt() const {return mSceneNode.size();}
	virtual unsigned long long GetAccelLeafCnt() const {return mKDLeafData.size();}
	virtual const KBBox& GetSceneBBox() const {return mSceneBBox;}
	virtual void GetKDBuildTimeStatistics(DWORD& kd_build, DWORD& gen_accel) const;
	virtual size_t CalcGeomDataSize() const;
	virtual void FinalizeKDTree(KAccelTriangleOpt1r4t* pGeomBuffer);
	
	// These functions will modify scene, BE CAREFUL!!!
	void AddKDNode(const KD_Node& node, UINT32& out_idx);
	void SetKNodeChild(UINT32 nodeIdx, UINT32 childIdx, bool left_or_right, bool isLeaf);
};
