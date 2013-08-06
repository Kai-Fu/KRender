#include "KKdTreeScene.h"
#include "../intersection/intersect_tri_bbox.h"
#include "../intersection/intersect_ray_tri.h"
#include "../intersection/intersect_ray_bbox.h"
#include <assert.h>
#include <algorithm>
#include "../util/HelperFunc.h"
#include "../util/triangle_filter.h"
#include "../entry/Constants.h"


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
					if (TriIntersectBBox(triPos, *clamp_box)) {
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
			leafData.tri_cnt = cnt;
			leafData.tri_list.leaf_triangles = leafTriIdx;

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
	int det_axis;
	KBBox left_bbox(bbox), right_bbox(bbox);
	if (fabs(det[0]) >= fabs(det[1]) && fabs(det[0]) >= fabs(det[2])) {
		// Split X axis
		det_axis = 0;
		node.flag |= eSplitX;
	}
	else if (fabs(det[1]) >= fabs(det[0]) && fabs(det[1]) >= fabs(det[2])) {
		// Split Y axis
		det_axis = 1;
		node.flag |= eSplitY;
	}
	else {
		// Split Z axis
		det_axis = 2;
		node.flag |= eSplitZ;
	}
	// 4. Calculate the splitting position
	{
		// Use the middle plan splitting, may be used for testing
		//det_value = (bbox.mMax[det_axis] + bbox.mMin[det_axis]) * 0.5f;
		det_value = CalcuSplittingPosition(
			triangles, cnt, 
			bbox,
			&mTempDataForKD->mTriBBox[0],
			det_axis,
			mTempDataForKD->mPigeonHoles[splitData.in_splitThreadIdx]);
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

	if ((float)stride_cnt >= (float)cnt * mNodeSplitThreshhold ) {
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

	mSceneNode.Append(*(const KD_Node_NoBBox*)&node);
	out_idx = (UINT32)mSceneNode.size() - 1;

	if (!(node.flag & eNoBBox))
		mSceneNode.AppendRawData(&node.bbox, sizeof(KBBoxOpt));

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

size_t KAccelStruct_KDTree::CalcGeomDataSize() const
{
#ifdef USE_PACKED_PRIMITIVE
	// First calculate the total geometry buffer's size
	size_t geomBufferSize = 0;
	for (UINT32 leaf_i = 0; leaf_i < mKDLeafData.size(); ++leaf_i) {
		const KD_LeafData& leafData = mKDLeafData[leaf_i];
		UINT32 total_size = leafData.tri_cnt + 3;
		geomBufferSize += (total_size >> 2);
	}
	return geomBufferSize;
#else
	return 0;
#endif
}

void KAccelStruct_KDTree::FinalizeKDTree(KAccelTriangleOpt1r4t* pGeomBuffer)
{

#ifdef USE_PACKED_PRIMITIVE

	// Fill the geometry buffer
	std::vector<KAccelTriangleOpt> kuv_tri[3];
	size_t currentFillPos = 0;
	size_t filled_tri4 = 0;
	for (UINT32 leaf_i = 0; leaf_i < mKDLeafData.size(); ++leaf_i) {
		KD_LeafData& leafData = mKDLeafData[leaf_i];

		for (UINT32 i = 0; i < 3; ++i)
			kuv_tri[i].resize(0);

		for (UINT32 tri_i = 0; tri_i < leafData.tri_cnt; ++tri_i) {
			UINT32 idx = leafData.tri_list.leaf_triangles[tri_i];
			KAccelTriangleOpt accelTriOpt;
			KTriVertPos2 triPos;
			mpSourceScene->GetAccelTriPos(mAccelTriangle[idx], triPos);
			PrecomputeAccelTri(triPos,idx, accelTriOpt);
			kuv_tri[accelTriOpt.k].push_back(accelTriOpt);
		}

		UINT32 total_size = 0;
		for (int gi = 0; gi < 3; ++gi) {
			total_size += (UINT32)kuv_tri[gi].size();
		}
		total_size += 3;
		UINT32 tri4cnt = (total_size >> 2);
		filled_tri4 += tri4cnt;
		KAccelTriangleOpt1r4t* ret = &pGeomBuffer[currentFillPos];

		leafData.tri_cnt = tri4cnt;
		leafData.tri_list.accel_tri4 = ret;
		currentFillPos += tri4cnt;
		
		UINT32 curPos = 0;
		FillAccelTri1r4t(kuv_tri[0], ret, 0, false);
		curPos += (UINT32)kuv_tri[0].size();
		FillAccelTri1r4t(kuv_tri[1], ret, curPos, false);
		curPos += (UINT32)kuv_tri[1].size();
		FillAccelTri1r4t(kuv_tri[2], ret, curPos, true);
		curPos += (UINT32)kuv_tri[2].size();

		if ((curPos % 4) != 0) {
			UINT32 last_bias = (curPos % 4) - 1;
			UINT32 last_idx = tri4cnt - 1;
			KAccelTriangleOpt1r4t* acc_tri = ret;
			for (UINT32 i = last_bias + 1; i < 4; ++i) {
				vec4_f(acc_tri[last_idx].b_d, i) = vec4_f(acc_tri[last_idx].b_d, last_bias);
				vec4_f(acc_tri[last_idx].b_nu, i) = vec4_f(acc_tri[last_idx].b_nu, last_bias);
				vec4_f(acc_tri[last_idx].b_nv, i) = vec4_f(acc_tri[last_idx].b_nv, last_bias);

				vec4_f(acc_tri[last_idx].c_d, i) = vec4_f(acc_tri[last_idx].c_d, last_bias);
				vec4_f(acc_tri[last_idx].c_nu, i) = vec4_f(acc_tri[last_idx].c_nu, last_bias);
				vec4_f(acc_tri[last_idx].c_nv, i) = vec4_f(acc_tri[last_idx].c_nv, last_bias);

				vec4_f(acc_tri[last_idx].n_d, i) = vec4_f(acc_tri[last_idx].n_d, last_bias);
				vec4_f(acc_tri[last_idx].n_u, i) = vec4_f(acc_tri[last_idx].n_u, last_bias);
				vec4_f(acc_tri[last_idx].n_v, i) = vec4_f(acc_tri[last_idx].n_v, last_bias);

				vec4_i(acc_tri[last_idx].k, i) = vec4_i(acc_tri[last_idx].k, last_bias);

				vec4_i(acc_tri[last_idx].tri_id, i) = vec4_i(acc_tri[last_idx].tri_id, last_bias);
				vec4_f(acc_tri[last_idx].E_F, i) = vec4_f(acc_tri[last_idx].E_F, last_bias);
			}
		}
	}

	mLeafIdxMemPool.Reset();
	
#endif
	
}

void KAccelStruct_KDTree::FillAccelTri1r4t(const std::vector<KAccelTriangleOpt>& kuv_tri, KAccelTriangleOpt1r4t* acc_tri, UINT32 offset_tri4, bool pad_end) const
{
	if (kuv_tri.empty())
		return;

	const UINT32 shuffle_mask[3] = {0x03020100, 0x07060504, 0x0b0a0908};
	UINT32 k = shuffle_mask[kuv_tri[0].k];
	UINT32 last_idx = INVALID_INDEX;
	UINT32 last_bias = INVALID_INDEX;

	for (UINT32 i = 0; i < kuv_tri.size(); ++i) {
		
		const KAccelTriangleOpt& tri = kuv_tri[i];
		UINT32 r_i = (offset_tri4 + i) >> 2;
		UINT32 bias = (offset_tri4 + i) % 4;
		last_bias = bias;
		last_idx = r_i;
		vec4_f(acc_tri[r_i].b_d, bias) = tri.b_d;
		vec4_f(acc_tri[r_i].b_nu, bias) = tri.b_nu;
		vec4_f(acc_tri[r_i].b_nv, bias) = tri.b_nv;

		vec4_f(acc_tri[r_i].c_d, bias) = tri.c_d;
		vec4_f(acc_tri[r_i].c_nu, bias) = tri.c_nu;
		vec4_f(acc_tri[r_i].c_nv, bias) = tri.c_nv;

		vec4_f(acc_tri[r_i].n_d, bias) = tri.n_d;
		vec4_f(acc_tri[r_i].n_u, bias) = tri.n_u;
		vec4_f(acc_tri[r_i].n_v, bias) = tri.n_v;
		
		vec4_i(acc_tri[r_i].k, bias) = k;

		vec4_i(acc_tri[r_i].tri_id, bias) = tri.tri_id;
		// How this magic value is here?
		// This is calculated by assuming the image size is 10000*10000, and the tan(half of fov) is 0.5
		vec4_f(acc_tri[r_i].E_F, bias) = 0.000001f / tri.E_F;

	}

}

bool KAccelStruct_KDTree::IntersectLeaf(UINT32 idx, const KRay& ray, float cur_t, IntersectContext& ctx) const
{
	const KD_LeafData& leafData = mKDLeafData[idx];
	bool ret = false;

	float t0 = 0, t1 = FLT_MAX;
	if (!IntersectBBox(ray, leafData.bbox,t0, t1))
		return false;
	if (t0 > ctx.t)
		return false; // ok, the required distance is reached

	// we need to make sure the hit point is inside this node's bounding box
	// t1 is the possible furthest point
	float old_t = ctx.t;
	ctx.t = (old_t < t1 ? old_t : t1);

#ifdef USE_PACKED_PRIMITIVE

	if (RayIntersect4Tri(ray, leafData.tri_list.accel_tri4, leafData.tri_cnt, ctx)) {
		ret = true;
	}

#else
	for (UINT32 i = 0; i < leafData.tri_cnt; ++i) {
		UINT32 tri_idx = leafData.tri_list.leaf_triangles[i];
		KTriVertPos2 triPos;
		mpSourceScene->GetAccelTriPos(mAccelTriangle[tri_idx], triPos);

		if (RayIntersect(ray, triPos, cur_t, tri_idx, ctx)) {
		
			// a new nearer triangle is hit
			if (ctx.bbox_node_idx != ray.mExcludeBBoxNode || ctx.tri_id != ray.mExcludeTriID)
				ret = true;
		}
	}
	
#endif

	// If no triangle get hit, then I should restore it back.
	if (ret)
		ctx.kd_leaf_idx = idx;
	else { 
		ctx.walkVec = ray.GetOrg() + ray.GetDir() * t1;
		ctx.t = old_t;
	}

	return ret;
}

bool KAccelStruct_KDTree::IntersectNode(UINT32 idx, const KRay& ray, float cur_t, IntersectContext& ctx) const
{
	float t0 = 0, t1 = FLT_MAX;
	bool bUpdateWalk = false;
	const KD_Node_NoBBox& node = mSceneNode[idx];
	UINT32 det_axis = node.flag & eSplitAxisMask;

	if (!(node.flag & eNoBBox)) {
		const KD_Node* pKDNode = (const KD_Node*)&node;
		if (!IntersectBBox(ray, pKDNode->bbox, t0, t1))
			return false;
		bUpdateWalk = true;
	}

	if (t0 > ctx.t)
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
			if (IntersectLeaf(next_node0, ray, cur_t, ctx))
				return true;
		}
		else if (IntersectNode(next_node0, ray, cur_t, ctx)) {
			return true;
		}
	}


	
	// No triangle is hit in the fisrt child node, here we do some check to see 
	// whether we need to test the other child node.
	float hit_pt = ray.GetOrg()[det_axis] + ray.GetDir()[det_axis]*t1;
	if ((hit_pt - node.split_value)*det_sign > mSceneEpsilon)
		return false;
	
	if (next_node1 != INVALID_INDEX) {
		if (child_flag1)
			return IntersectLeaf(next_node1, ray, cur_t, ctx);
		else
			return IntersectNode(next_node1, ray, cur_t, ctx);
	}

	return false;
}

bool KAccelStruct_KDTree::IntersectRay_KDTree(const KRay& ray, float cur_t, IntersectContext& ctx) const
{
	ctx.walkVec = ray.GetOrg();
	if (mRootNode != INVALID_INDEX) {
		if (!mSceneNode.empty())
			return IntersectNode(mRootNode, ray, cur_t, ctx);
		else
			return IntersectLeaf(mRootNode, ray, cur_t, ctx);
	}
	else
		return false;
}


bool KAccelStruct_KDTree::IntersectRay_BruteForce(const KRay& ray, float cur_t, IntersectContext& ctx) const
{
	bool ret = false;
	for (UINT32 i = 0; i < mAccelTriangle.size(); ++i) {
		KTriVertPos2 triPos;
		mpSourceScene->GetAccelTriPos(mAccelTriangle[i], triPos);

		if (RayIntersect(ray, triPos, 0, i, ctx)) {
			ctx.tri_id = i;
			ret = true;
		}
	}
	return ret;
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

#ifdef USE_PACKED_PRIMITIVE
	
#else
	mLeafIdxMemPool.Reset();
#endif

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
