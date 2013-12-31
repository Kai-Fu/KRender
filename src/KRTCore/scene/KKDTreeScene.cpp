#include "KKdTreeScene.h"
#include "../intersection/intersect_tri_bbox.h"
#include "../intersection/intersect_ray_tri.h"
#include "../intersection/intersect_ray_bbox.h"
#include <assert.h>
#include <algorithm>
#include "../util/HelperFunc.h"
#include "../util/triangle_filter.h"
#include "../entry/Constants.h"


KAccelStruct_KDTree::PFN_RayIntersectStaticTriArray KAccelStruct_KDTree::s_pPFN_RayIntersectStaticTriArray = NULL;
KAccelStruct_KDTree::PFN_RayIntersectAnimTriArray KAccelStruct_KDTree::s_pPFN_RayIntersectAnimTriArray = NULL;

KAccelStruct_KDTree::KAccelStruct_KDTree(const KScene* scene)
{
	m_kdBuildTime = 0;
	m_buildAccelTriTime = 0;
	mProcessorCnt = GetConfigedThreadCount();
	mpSourceScene = NULL;
	mpSourceScene = scene;
	ResetScene();
}

KAccelStruct_KDTree::~KAccelStruct_KDTree()
{
	ResetScene();
}

const KScene* KAccelStruct_KDTree::GetSource() const
{
	return mpSourceScene;
}

void SceneSplitTask::Execute()
{
	bool isLeaf = false;
	KAccelStruct_KDTree::SplitData splitData;
	splitData.in_splitThreadIdx = splitThreadIdx;
	UINT32 ret_value = pKDScene->SplitScene(
		triangles, cnt, 
		needBBox,
		&clamp_box, 
		splitData,
		depth, isLeaf);

	pKDScene->SetKNodeChild(destIndex, ret_value, 
		(split_flag & KAccelStruct_KDTree::eLeftChild) ? true : false, 
		isLeaf);
	pKDScene->mTempDataForKD->NotifySplitThreadComplete(splitThreadIdx);
}


UINT32 KAccelStruct_KDTree::SplitScene(UINT32* triangles, UINT32 cnt, 
								int needBBox,
								const KBBox* clamp_box, 
								const SplitData& splitData,
								int depth, bool& isLeaf)
{
	if (cnt == 0) return INVALID_INDEX;
	KD_Node node; 
	// 1. Calculate the bounding box of this node
	KBBox bbox;
	bool bShouldBeLeaf = (cnt <= mLeafTriCnt || depth == mMaxDepth);

	isLeaf = false;

	{
		// Compute the tight bounding box of the triangles
		if (clamp_box) {
			assert(triangles != NULL);
			
			if (depth < mTempDataForKD->mThreadLimit) {
				FilterByBBox(triangles, cnt, bbox, 
					*mTempDataForKD->mThreadBucket.get(),
					&mTempDataForKD->mTriBBox[0],
					this, clamp_box);

			}
			else {
				UINT32 i = 0;
				while (i < cnt) {
					UINT32 idx = triangles[i];
					KTriVertPos2 triPos;
					mpSourceScene->GetAccelTriPos(mAccelTriangle[idx], triPos);

					if (triPos.mIsMoving || TriIntersectBBox(triPos.mVertPos, *clamp_box)) {
						bbox.Add(mTempDataForKD->mTriBBox[idx]);
					}
					else {
						std::swap(triangles[i], triangles[cnt - 1]);
						--cnt;
						continue;
					}

					++i;
				}
				bbox.ClampBBox(*clamp_box);
			}
		}
		else {
			if (depth < mTempDataForKD->mThreadLimit) {
				CalcuTriangleArrayBBox(triangles, cnt, bbox, 
					*mTempDataForKD->mThreadBucket.get(), &mTempDataForKD->mTriBBox[0],
					this);
				
			}
			else {
				if (triangles) {
					for (UINT32 i = 0; i < cnt; ++i) {
						UINT32 idx = triangles[i];
						bbox.Add(mTempDataForKD->mTriBBox[idx]);
					}
				}
				else {
					for (UINT32 i = 0; i < cnt; ++i) 
						bbox.Add(mTempDataForKD->mTriBBox[i]);
				}
			}
		}
	}

	if (bbox.IsEmpty()) return INVALID_INDEX;

	bool bShrinkBBox = false;
	if (clamp_box) {
		bShrinkBBox = !(bbox.IsInside(clamp_box->mMax) && bbox.IsInside(clamp_box->mMin));
		if (bShrinkBBox) needBBox = 0;
	}

	node.bbox = bbox;

	bool bForceLeafNode = false;
	int tryOtherSplitAxis = 0;
	
	int det_axis;

FORCE_LEAF_NODE:
	node.flag = (!(needBBox % 6) ? 0 : eNoBBox);
	++needBBox;
	// 2. If the triangle count is less than the limit, then add all the triangle index into leaf node
	if (bForceLeafNode || bShouldBeLeaf) {
		isLeaf = true;
		UINT32* leafTriIdx = NULL;
		// Create triangle list here
		if (cnt > 0) {
			leafTriIdx = (UINT32*)mLeafIdxMemPool.Alloc(sizeof(UINT32)*cnt);
			if (triangles) {
				memcpy(leafTriIdx, triangles, sizeof(UINT32)*cnt);
			}
			else {
				for (UINT32 i = 0; i < cnt; ++i)
					leafTriIdx[i] = i;
			}
			KD_LeafData leafData;
			leafData.bbox = bbox;
			leafData.box_norm.InitFromBBox(bbox);
			leafData.tri_cnt = cnt;
			leafData.tri_list.leaf_triangles = leafTriIdx;
			leafData.hasAnim = false;
			// Check whether this tri-leaf node contains animation
			for (UINT32 i = 0; i < cnt; ++i) {
				if (mpSourceScene->IsTriPosAnimated(mAccelTriangle[i])) {
					leafData.hasAnim = true;
					break;
				}
			}

			EnterSpinLockCriticalSection(mTempDataForKD->mLeafAddingCS);
			mTotalLeafTriCnt += cnt;
			mKDLeafData.push_back(leafData);
			UINT32 ret = (UINT32)mKDLeafData.size() - 1;

			LeaveSpinLockCriticalSection(mTempDataForKD->mLeafAddingCS);
			return ret;
		}
		else {

			return INVALID_INDEX;
		}
	}
	
	// 3. According to the bounding box, determine how to split this node
	KVec3 det = bbox.mMax - bbox.mMin;
	float det_value;
	bool  useMiddleSplit = false;
	KBBox left_bbox(bbox), right_bbox(bbox);
	if (tryOtherSplitAxis) {
		det_axis = (det_axis + tryOtherSplitAxis) % 3;
		if (tryOtherSplitAxis > 2) {
			det_axis = bbox.LongestAxis();
			useMiddleSplit = true;
		}
	}
	else {
		
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
	}

	switch (det_axis) {
	case 0: node.flag |= eSplitX; break;
	case 1: node.flag |= eSplitY; break;
	case 2: node.flag |= eSplitZ; break;
	}
	// 4. Calculate the splitting position
	{
		// Use the middle plan splitting, may be used for testing
		if (useMiddleSplit)
			det_value = (bbox.mMax[det_axis] + bbox.mMin[det_axis]) * 0.5f;
		else {
			det_value = CalcuSplittingPosition(
				triangles, cnt, 
				bbox,
				&mTempDataForKD->mTriBBox[0],
				det_axis,
				mTempDataForKD->mPigeonHoles[splitData.in_splitThreadIdx]);
		}
	}
	node.split_value = det_value;
	left_bbox.mMin[det_axis] = det_value;
	right_bbox.mMax[det_axis] = det_value;

	// Allocate triangle index buffer for children nodes
	UINT32* tempTriIdxBuf = NULL;
	UINT32* left_triangles = NULL;
	UINT32* right_triangles = NULL;
	UINT32 addedThreadIdx[2] = {INVALID_INDEX, INVALID_INDEX};
	// If there're available CPU in thread pool, bNeedSpawnThread will be true
	bool bNeedSpawnThread = (cnt > (LEAF_TRIANGLE_CNT*5) && 
		splitData.in_splitThreadIdx != 0 &&
		mTempDataForKD->mActiveSplitThreadCnt < (long)mTempDataForKD->mCPUCnt);
	bool bIsThreadSplitPoint = (mTempDataForKD->mThreadLimit == depth);
	bool bKickSubThread = false;
	if (bIsThreadSplitPoint) {
		addedThreadIdx[0] = mTempDataForKD->mThreadPool->PopIdleThreadIndex();
		addedThreadIdx[1] = mTempDataForKD->mThreadPool->PopIdleThreadIndex();
		assert(addedThreadIdx[0] != INVALID_INDEX && addedThreadIdx[1] != INVALID_INDEX);

		left_triangles = mTempDataForKD->AcquireTempTriIdxBuf(addedThreadIdx[0] + 1, depth, cnt);
		right_triangles = mTempDataForKD->AcquireTempTriIdxBuf(addedThreadIdx[1] + 1, depth, cnt);
		bNeedSpawnThread = true;

	} else if (bNeedSpawnThread) {
		addedThreadIdx[0] = mTempDataForKD->mThreadPool->PopIdleThreadIndex();
		if (addedThreadIdx[0] != INVALID_INDEX) {
			left_triangles = mTempDataForKD->AcquireTempTriIdxBuf(addedThreadIdx[0] + 1, depth, cnt);
			right_triangles = mTempDataForKD->AcquireTempTriIdxBuf(splitData.in_splitThreadIdx, depth, cnt);
			bKickSubThread = true;
		}
		else
			bNeedSpawnThread = false;
	}
	if (!bNeedSpawnThread) {
		tempTriIdxBuf = mTempDataForKD->AcquireTempTriIdxBuf(splitData.in_splitThreadIdx, depth, cnt*2);
		left_triangles = tempTriIdxBuf;
		right_triangles = left_triangles + cnt;
	}

	UINT32 cnt0 = 0;
	UINT32 cnt1 = 0;
	UINT32 stride_cnt = 0; // triangle count that stride over two sides
	for (UINT32 i = 0; i < cnt; ++i) {
		UINT32 idx = (triangles ? triangles[i] : i);
		const KBBox& tri_box = mTempDataForKD->mTriBBox[idx];
		if (tri_box.mMin[det_axis] > det_value) {
			// this triangle is in + side
			left_triangles[cnt0++] = idx;

		}
		else if (tri_box.mMax[det_axis] < det_value) {
			// this triangle is in - side
			right_triangles[cnt1++] = idx;
		}
		else {
			// this triangle stride over two sides
			left_triangles[cnt0++] = idx;
			right_triangles[cnt1++] = idx;
			++stride_cnt;
		}
	}

	if (0 == stride_cnt) {
		++mPerfectsplitCnt;
		node.flag |= ePerfectSplit;
	}

	// If the splitting produces more striding triangles than the threshold, just try several other splitting methods.
	// If all the splitting methods have been tried and it still cannot pass the threshold, just accept the middle splitting method
	if ((float)stride_cnt >= (float)cnt * mNodeSplitThreshhold &&
		tryOtherSplitAxis <= 2) {
		if (cnt <= mLeafTriCnt*10) {
			bForceLeafNode = true;
		}
		else
			++tryOtherSplitAxis;

		for (int i = 0; i < 2; ++i) {
			if (addedThreadIdx[i] != INVALID_INDEX) 
				mTempDataForKD->mThreadPool->ReleaseScheduledThread(addedThreadIdx[i]);
		}

		goto FORCE_LEAF_NODE;
	}

	if (cnt0 == cnt || cnt1 == cnt) {
		// Ok...I surrender
		bForceLeafNode = true;
		for (int i = 0; i < 2; ++i) {
			if (addedThreadIdx[i] != INVALID_INDEX) 
				mTempDataForKD->mThreadPool->ReleaseScheduledThread(addedThreadIdx[i]);
		}

		goto FORCE_LEAF_NODE;
	}

	UINT32 ret;
	if (bNeedSpawnThread) {
		
		AddKDNode(node, ret);
		SceneSplitTask* pSplitTask = NULL;
		if (cnt0 > 0) {

			pSplitTask = &mTempDataForKD->mSplitTasks[addedThreadIdx[0]];
			pSplitTask->pKDScene = this;
			pSplitTask->triangles = left_triangles;
			pSplitTask->cnt = cnt0;
			pSplitTask->split_flag = eLeftChild;
			pSplitTask->clamp_box = left_bbox;
			pSplitTask->needBBox = needBBox;
			pSplitTask->depth = depth + 1;
			pSplitTask->destIndex = ret;
			pSplitTask->splitThreadIdx = addedThreadIdx[0] + 1; // Note: this thread is in the pool, main thread isn't.
			// Notify the split thread started before kick the thread
			mTempDataForKD->NotifySplitThreadAdded();
			if (bKickSubThread)
				mTempDataForKD->mThreadPool->KickIdleThread(addedThreadIdx[0]);
		}
		else {
			mTempDataForKD->mThreadPool->ReleaseScheduledThread(addedThreadIdx[0]);
			SetKNodeChild(ret, INVALID_INDEX, true, false);
		}	

		pSplitTask = NULL;
		if (cnt1 > 0) {
			
			if (addedThreadIdx[1] != INVALID_INDEX) {
				pSplitTask = &mTempDataForKD->mSplitTasks[addedThreadIdx[1]];
				pSplitTask->pKDScene = this;
				pSplitTask->triangles = right_triangles;
				pSplitTask->cnt = cnt1;
				pSplitTask->split_flag = eRightChild;
				pSplitTask->clamp_box = right_bbox;
				pSplitTask->needBBox = needBBox;
				pSplitTask->depth = depth + 1;
				pSplitTask->destIndex = ret;
				pSplitTask->splitThreadIdx = addedThreadIdx[1] + 1; // Note: this thread is in the pool, main thread isn't.
				// Notify the split thread started before kick the thread
				mTempDataForKD->NotifySplitThreadAdded();
			}
			else {
				bool isLeafNode = false;
				UINT32 right_idx = SplitScene(right_triangles, cnt1, needBBox, &right_bbox, splitData, depth + 1, isLeafNode);
				SetKNodeChild(ret, right_idx, false, isLeafNode);
			}
		}
		else {
			if (addedThreadIdx[1] != INVALID_INDEX) 
				mTempDataForKD->mThreadPool->ReleaseScheduledThread(addedThreadIdx[1]);
			SetKNodeChild(ret, INVALID_INDEX, false, false);
		}
	}
	else {
		bool isLeafNode = false;
		node.left_child = SplitScene(left_triangles, cnt0, needBBox, &left_bbox, splitData, depth + 1, isLeafNode);
		node.flag |= (isLeafNode ? eLeftChild : 0);
		node.right_child = SplitScene(right_triangles, cnt1, needBBox, &right_bbox, splitData, depth + 1, isLeafNode);
		node.flag |= (isLeafNode ? eRightChild : 0);
		AddKDNode(node, ret);
	}

	return ret;
}

void KAccelStruct_KDTree::AddKDNode(const KD_Node& node, UINT32& out_idx)
{
	EnterSpinLockCriticalSection(mTempDataForKD->mNodeAddingCS);

	UINT32 oldSize = (UINT32)mSceneNode.size();
	out_idx = oldSize;
	mSceneNode.resize(oldSize + sizeof(KD_Node_NoBBox));
	*(KD_Node_NoBBox*)&mSceneNode[oldSize] = node;
	if (!(node.flag & eNoBBox)) {
		oldSize = (UINT32)mSceneNode.size();
		mSceneNode.resize(oldSize + sizeof(KBBox));
		*(KBBox*)&mSceneNode[oldSize] = node.bbox;
	}

	LeaveSpinLockCriticalSection(mTempDataForKD->mNodeAddingCS);
}

void KAccelStruct_KDTree::SetKNodeChild(UINT32 nodeIdx, UINT32 childIdx, bool left_or_right, bool isLeaf)
{	
	EnterSpinLockCriticalSection(mTempDataForKD->mNodeAddingCS);
	KD_Node& node = *(KD_Node*)&mSceneNode[nodeIdx];
	if (isLeaf)
		node.flag |= (left_or_right ? eLeftChild : eRightChild);

	if (left_or_right)
		node.left_child = childIdx;
	else
		node.right_child = childIdx;
	LeaveSpinLockCriticalSection(mTempDataForKD->mNodeAddingCS);
}

void KAccelStruct_KDTree::PrecomputeTriangleBBox()
{
	KTriVertPos2 triVertPos;
	mTempDataForKD->mTriBBox.resize(mAccelTriangle.size());
	for (size_t i = 0; i < mAccelTriangle.size(); ++i) {
		mpSourceScene->GetAccelTriPos(mAccelTriangle[i], triVertPos);
		KBBox bbox(triVertPos);
		mTempDataForKD->mTriBBox[i] = bbox;
	}
}

int KAccelStruct_KDTree::PrepareKDTree()
{
	// Start build kd-tree...
	mTotalLeafTriCnt = 0;
	mPerfectsplitCnt = 0;
	double time_elapse;

	KTimer stop_watch(true);
	mpSourceScene->InitAccelTriangleCache(mAccelTriangle);
	time_elapse = stop_watch.Stop();
	m_buildAccelTriTime = DWORD(time_elapse*1000.0);

	UINT32* triangles = NULL;	// Null indicates an array of 0,1,2,3...
	//---------------Start building kd-tree---------------------
	stop_watch.Start();
	size_t apprxmNodeCnt = size_t((float)mAccelTriangle.size() * 0.18f);
	mSceneNode.reserve(apprxmNodeCnt);
	mKDLeafData.reserve(apprxmNodeCnt);
	bool isLeaf = false;
	{
		// Split the scene and do the kd-tree building
		mTempDataForKD.reset(new DATA_FOR_KD_BUILD);
		PrecomputeTriangleBBox();

		SplitData splitData;
		splitData.in_splitThreadIdx = 0; // the main split thread
		mRootNode = SplitScene(triangles, (UINT32)mAccelTriangle.size(), 0, NULL, splitData, 0, isLeaf);
		// Clear the temporary memory allocated in main thread
		//mTempDataForKD->mSplitThreadTriArrayLevel[0].clear();
		if (mTempDataForKD->mThreadPool.get())
			mTempDataForKD->mThreadPool->KickScheduledThreads();
		
		// Main thread finished
		mTempDataForKD->NotifySplitThreadComplete(INVALID_INDEX);
		mTempDataForKD->mSplitThreadTriArrayLevel[0].clear();
		if (mTempDataForKD->mThreadPool.get())
			mTempDataForKD->mThreadPool->WaitForAll();
	
		mTempDataForKD.reset();
	}

	time_elapse = stop_watch.Stop();
	m_kdBuildTime = DWORD(time_elapse * 1000.0);
	//---------------End of building kd-tree--------------------

	if (mRootNode != INVALID_INDEX) {
		if (!mSceneNode.empty()) {
			const KD_Node* pRootNode = (const KD_Node*)&mSceneNode[mRootNode];
			assert(!(pRootNode->flag & eNoBBox));
			mSceneBBox = pRootNode->bbox;
		}
		else
			mSceneBBox = mKDLeafData[mRootNode].bbox;
		KVec3 diagnol = mSceneBBox.mMax - mSceneBBox.mMin;
		mSceneEpsilon = nvmath::length(diagnol) * 1.0E-6f;
	}

	return mRootNode;
}

bool KAccelStruct_KDTree::IntersectLeaf(UINT32 idx, const KRay& ray, TracingInstance* inst, IntersectContext& ctx) const
{
	const KD_LeafData& leafData = mKDLeafData[idx];
	bool ret = false;

	double t0 = 0, t1 = FLT_MAX;
	if (!IntersectBBox(ray, leafData.bbox, t0, t1))
		return false;
	if (t0 > ctx.ray_t)
		return false; // ok, the required distance is reached

	// we need to make sure the hit point is inside this node's bounding box
	// t1 is the possible furthest point
	double old_t = ctx.ray_t;
	ctx.ray_t = (old_t < t1 ? old_t : t1);

	KVec3 tempRayOrg = ToVec3f(ray.GetOrg() + ray.GetDir() * t0);
	KVec3 tempRayDir = ToVec3f(ray.mNormDir);
	leafData.box_norm.ApplyToRay(tempRayOrg, tempRayDir);

	double tScale = ray.mDirLen * leafData.box_norm.mRcpScaleLen;

#if 0
	for (UINT32 i = 0; i < leafData.tri_cnt; ++i) {
		UINT32 tri_idx = leafData.tri_list.leaf_triangles[i];
		KTriVertPos2 triPos;
		mpSourceScene->GetAccelTriPos(mAccelTriangle[tri_idx], triPos, &leafData.box_norm);
		RayIntersect((const float*)&tempRayOrg, (const float*)&tempRayDir, triPos, inst->mCameraContext.inMotionTime, inst->mTmpRayTriIntsct[i]);
	}
#else

	UINT32 triStep = leafData.hasAnim ? 18 : 9;
	int SIMD_tri_cnt = leafData.tri_cnt / inst->mSIMD_Width;
	if (leafData.tri_cnt % inst->mSIMD_Width != 0)
		++SIMD_tri_cnt;

	bool needUpdate = true;
	UINT64 leafId = ((UINT64)inst->mCurBVHIndex << 32) + idx;
	UINT32 triPosDataSize = leafData.tri_cnt *sizeof(float) * triStep;
	UINT32 triIdDataSize = leafData.tri_cnt * sizeof(int);
	UINT32 totalDataSize = triPosDataSize + triIdDataSize;
	UINT32 paddingTriPosSize = SIMD_tri_cnt * inst->mSIMD_Width * sizeof(float) * triStep;;
	UINT32 paddingTriIdSize = SIMD_tri_cnt * inst->mSIMD_Width * sizeof(int);
	BYTE* pSwizzledTriData = (BYTE*)inst->mCachedTriPosData.LRU_OpenEntry(leafId, paddingTriPosSize + paddingTriIdSize, needUpdate);
	int* pSwizzledTriIdData = (int*)(pSwizzledTriData + paddingTriPosSize);
	

	if (needUpdate) {
		if (inst->mTmpTriDataArray.size() < totalDataSize)
			inst->mTmpTriDataArray.resize(totalDataSize);
		BYTE* pCachedTriData = &inst->mTmpTriDataArray[0];
		float* pCachedTriPosData = (float*)pCachedTriData;
		int* pCacheTriIdData = (int*)(pCachedTriData + triPosDataSize);

		float* pCurTriData = pCachedTriPosData;
		for (UINT32 i = 0; i < leafData.tri_cnt; ++i) {
			UINT32 tri_idx = leafData.tri_list.leaf_triangles[i];
			mpSourceScene->GetTriPosData(mAccelTriangle[tri_idx], leafData.hasAnim, pCurTriData, &leafData.box_norm);
			pCurTriData += triStep;
			pCacheTriIdData[i] = (int)tri_idx;
		}

		SwizzleForSIMD(pCachedTriData, pSwizzledTriData, inst->mSIMD_Width, sizeof(float), triStep*sizeof(float), leafData.tri_cnt);
		SwizzleForSIMD(pCacheTriIdData, pSwizzledTriIdData, inst->mSIMD_Width, sizeof(int), sizeof(int), leafData.tri_cnt);
	}

	if (leafData.hasAnim) {
		s_pPFN_RayIntersectAnimTriArray(
			(const float*)&tempRayOrg, (const float*)&tempRayDir, 
			inst->mCameraContext.inMotionTime, 
			(float*)pSwizzledTriData, pSwizzledTriIdData, 
			inst->mpTUV_SIMD, inst->mpHitIdx_SIMD, 
			SIMD_tri_cnt, (int)ray.mExcludeTriID);
	}
	else {
		s_pPFN_RayIntersectStaticTriArray(
			(const float*)&tempRayOrg, (const float*)&tempRayDir, 
			(float*)pSwizzledTriData, pSwizzledTriIdData, 
			inst->mpTUV_SIMD, inst->mpHitIdx_SIMD, 
			SIMD_tri_cnt, (int)ray.mExcludeTriID);
	}

#endif
	double min_ray_t = (ctx.ray_t - t0) * tScale;
	int min_idx = INVALID_INDEX;
	for (int i = 0; i < inst->mSIMD_Width; ++i) {
		if (inst->mpTUV_SIMD[i] < min_ray_t) {
			min_ray_t = inst->mpTUV_SIMD[i];
			min_idx = i;
			ret = true;
		}
	}

	// If no triangle get hit, then I should restore it back.
	if (ret) {
		ctx.ray_t = t0 + min_ray_t / tScale;
		ctx.u = inst->mpTUV_SIMD[inst->mSIMD_Width + min_idx];
		ctx.v = inst->mpTUV_SIMD[inst->mSIMD_Width*2 + min_idx];
		ctx.w = 1.0f - ctx.u - ctx.v;
		ctx.tri_id = inst->mpHitIdx_SIMD[min_idx];
		ctx.kd_leaf_idx = idx;
	}
	else { 
		ctx.walkVec = ToVec3f(ray.GetOrg() + ray.GetDir() * t1);
		ctx.ray_t = old_t;
	}

	return ret;
}

bool KAccelStruct_KDTree::IntersectNode(UINT32 idx, const KRay& ray, TracingInstance* inst, IntersectContext& ctx) const
{
	double t0 = 0, t1 = FLT_MAX;
	const KD_Node_NoBBox& node = *(const KD_Node_NoBBox*)&mSceneNode[idx];
	UINT32 det_axis = node.flag & eSplitAxisMask;

	if (!(node.flag & eNoBBox)) {
		const KBBox* pNodeBBox = (const KBBox*)&mSceneNode[idx + sizeof(KD_Node_NoBBox)];
		if (!IntersectBBox(ray, *pNodeBBox, t0, t1))
			return false;
	}

	if (t0 > ctx.ray_t)
		return false; // ok, the required distance is reached
		
	UINT32 next_node0 = INVALID_INDEX;
	UINT32 next_node1 = INVALID_INDEX;
	UINT32 child_flag0;
	UINT32 child_flag1;
	float det_sign = 1.0f;
	if (ctx.walkVec[det_axis] >= node.split_value) {
		next_node0 = node.left_child;
		child_flag0 = (node.flag & eLeftChild);		
		if (ray.mSign[det_axis]) {
			next_node1 = node.right_child;
			child_flag1 = (node.flag & eRightChild);
		}
	}
	else {
		det_sign = -1.0f;
		child_flag0 = (node.flag & eRightChild);
		next_node0 = node.right_child;
		if (!ray.mSign[det_axis]) {
			next_node1 = node.left_child;
			child_flag1 = (node.flag & eLeftChild);
		}
	}

	if (next_node0 != INVALID_INDEX) {
		if (child_flag0) {
			if (IntersectLeaf(next_node0, ray, inst, ctx))
				return true;
		}
		else if (IntersectNode(next_node0, ray, inst, ctx)) {
			return true;
		}
	}

	
	if (next_node1 != INVALID_INDEX) {
		if (child_flag1)
			return IntersectLeaf(next_node1, ray, inst, ctx);
		else
			return IntersectNode(next_node1, ray, inst, ctx);
	}

	return false;
}

bool KAccelStruct_KDTree::IntersectRay_KDTree(const KRay& ray, TracingInstance* inst, IntersectContext& ctx) const
{
	ctx.walkVec = ray.GetOrg();
	if (mRootNode != INVALID_INDEX) {
		if (!mSceneNode.empty())
			return IntersectNode(mRootNode, ray, inst, ctx);
		else
			return IntersectLeaf(mRootNode, ray, inst, ctx);
	}
	else
		return false;
}


void KAccelStruct_KDTree::InitAccelData()
{
	PrepareKDTree();
}

void KAccelStruct_KDTree::ResetScene()
{
	mNodeSplitThreshhold = 0.6f;
	mLeafTriCnt = LEAF_TRIANGLE_CNT;
	mMaxDepth = MAX_KD_DEPTH;
	mRootNode = INVALID_INDEX;

	mLeafIdxMemPool.Reset();

	mKDLeafData.clear();
	mSceneNode.clear();
}

const KTriDesc* KAccelStruct::GetAccelTriData(UINT32 tri_idx) const
{
	return &mAccelTriangle[tri_idx];
}

void KAccelStruct_KDTree::GetKDBuildTimeStatistics(DWORD& kd_build, DWORD& gen_accel) const
{
	kd_build = m_kdBuildTime;
	gen_accel = m_buildAccelTriTime;
}

KAccelStruct_KDTree::DATA_FOR_KD_BUILD::DATA_FOR_KD_BUILD() 
{
	mNodeAddingCS = 0; 
	mLeafAddingCS = 0; 
	mActiveSplitThreadCnt = 1;	 // the main thread counts for one.

	UINT32 cnt = GetConfigedThreadCount();
	mCPUCnt = cnt;
	mSplitThreadTriArrayLevel.resize(cnt + 1);
	mPigeonHoles.resize(cnt + 1);
	for (size_t i = 0; i < mSplitThreadTriArrayLevel.size(); ++i) {
		mSplitThreadTriArrayLevel[i].resize(MAX_KD_DEPTH);
	}

	if (cnt > 1) {
		mThreadLimit = 0;

		while (cnt >= 4) {
			++mThreadLimit;
			cnt >>= 1;
		}
		mSplitTasks.resize(mCPUCnt);
		mThreadPool.reset(new ThreadModel::ThreadPool(mCPUCnt));
		mThreadBucket.reset(new ThreadModel::ThreadBucket(mCPUCnt));
		
		for (UINT32 i = 0; i < mCPUCnt; ++i)
			mThreadPool->SetThreadTask(i, &mSplitTasks[i]);
	}
	else {
		mThreadLimit = -1;	// Should use single thread
	}
}

KAccelStruct_KDTree::DATA_FOR_KD_BUILD::~DATA_FOR_KD_BUILD() 
{
}

void KAccelStruct_KDTree::DATA_FOR_KD_BUILD::NotifySplitThreadAdded()
{
	atomic_increment(&mActiveSplitThreadCnt);
}

void KAccelStruct_KDTree::DATA_FOR_KD_BUILD::NotifySplitThreadComplete(UINT32 thread_idx)
{
	atomic_decrement(&mActiveSplitThreadCnt);
}

UINT32* KAccelStruct_KDTree::DATA_FOR_KD_BUILD::AcquireTempTriIdxBuf(UINT32 threadIdx, int depth, UINT32 cnt)
{
	TRI_IDX_ARRAY& tempBuf = mSplitThreadTriArrayLevel[threadIdx][depth];
	if (cnt > 0) {
		tempBuf.Resize(cnt);
		return tempBuf.pData;
	}
	else
		return NULL;
}
