#include "triangle_filter.h"
#include "thread_model.h"
#include "../intersection/intersect_tri_bbox.h"
#include <assert.h>


#define MERGE_DST_OVERFLOW	2
#define MERGE_SRC_OVERFLOW	0
#define MERGE_OK			1
class FilterByBBoxTask : public ThreadModel::IThreadTask
{
public:
	// input/output
	UINT32* ptri_idx;
	UINT32 input_cnt;
	UINT32 output_cnt;
	// output
	KBBox bbox;

	const KAccelStruct_KDTree* pscene;
	const KBBox* clamp_box;
	const KBBox* triBBox;

	virtual void Execute()
	{
		UINT32 i = 0;
		output_cnt = input_cnt;
		while (i < output_cnt) {
			UINT32 idx = ptri_idx[i];
			const KTriDesc* pAccelTri = pscene->GetAccelTriData(idx);
			KTriVertPos2 triPos;
			pscene->GetSource()->GetAccelTriPos(*pAccelTri, triPos);
			if (triPos.mIsMoving || TriIntersectBBox(triPos.mVertPos, *clamp_box)) {
				bbox.Add(triBBox[idx]);
			}
			else {
				std::swap(ptri_idx[i], ptri_idx[output_cnt - 1]);
				--output_cnt;
				continue;
			}

			++i;
		}
		bbox.ClampBBox(*clamp_box);
	}

	int MergeWith(FilterByBBoxTask& dest)
	{
		if (input_cnt > output_cnt) {

			int ret = MERGE_OK;
			UINT32 merge_cnt = input_cnt - output_cnt;
			if (merge_cnt >= dest.output_cnt) {
				ret = MERGE_DST_OVERFLOW;
				merge_cnt = dest.output_cnt;
			}
			for (UINT32 i = 0; i < merge_cnt; ++i) {
				ptri_idx[output_cnt + i] = dest.ptri_idx[dest.output_cnt - i - 1];
			}
			output_cnt += merge_cnt;
			dest.output_cnt -= merge_cnt;
			return ret;
		}
		else
			return MERGE_SRC_OVERFLOW;
	}
};


void FilterByBBox(UINT32* ptri_idx, UINT32& cnt, KBBox& out_bbox, 
				  ThreadModel::ThreadBucket& thread_bucket, 
				  const KBBox* pTriBBox,
				  const KAccelStruct_KDTree* pscene, const KBBox* clamp_box)
{
	std::vector<FilterByBBoxTask> tasks;
	UINT32 thread_cnt = thread_bucket.GetThreadCnt();
	tasks.resize(thread_cnt);
	UINT32 tri_cnt_thread = cnt / thread_cnt;
	for (UINT32 i = 0; i < thread_cnt; ++i) {
		tasks[i].pscene = pscene;
		tasks[i].clamp_box = clamp_box;
		tasks[i].triBBox = pTriBBox;
		tasks[i].input_cnt = tri_cnt_thread;
		tasks[i].ptri_idx = (ptri_idx + tri_cnt_thread*i);
		thread_bucket.SetThreadTask(i, &tasks[i]);
	}
	tasks[thread_cnt-1].input_cnt += (cnt % thread_cnt);
	
	thread_bucket.Run();

	cnt = 0;
	out_bbox.SetEmpty();
	for (UINT32 i = 0; i < thread_cnt; ++i) {
		cnt += tasks[i].output_cnt;
		out_bbox.Add(tasks[i].bbox);
	}
	UINT32 src_idx = 0;
	UINT32 dst_idx = thread_cnt-1;
	while (1) {
		if (src_idx == dst_idx)
			break;
		int ret = tasks[src_idx].MergeWith(tasks[dst_idx]);
		if (ret == MERGE_SRC_OVERFLOW)
			++src_idx;
		else if (ret == MERGE_DST_OVERFLOW)
			--dst_idx;
			

	}

}

class CalcuTriBBoxTask : public ThreadModel::IThreadTask
{
public:
	// input/output
	const UINT32* ptri_idx;
	UINT32 start_idx;
	UINT32 cnt;
	// output
	KBBox bbox;

	const KAccelStruct_KDTree* pscene;
	const KBBox* triBBox;

	virtual void Execute()
	{
		UINT32 end_idx = start_idx + cnt;
		if (ptri_idx) {
			for (UINT32 i = start_idx; i < end_idx; ++i) {
				UINT32 idx = ptri_idx[i];
				bbox.Add(triBBox[idx]);
			}
		}
		else {
			for (UINT32 i = start_idx; i < end_idx; ++i) 
				bbox.Add(triBBox[i]);
		}
	}
};

void CalcuTriangleArrayBBox(const UINT32* ptri_idx, UINT32 cnt, KBBox& out_bbox, 
							ThreadModel::ThreadBucket& thread_bucket, 
							const KBBox* pTriBBox,
							const KAccelStruct_KDTree* pscene)
{
	std::vector<CalcuTriBBoxTask> tasks;
	UINT32 thread_cnt = thread_bucket.GetThreadCnt();
	tasks.resize(thread_cnt);

	UINT32 tri_cnt_thread = cnt / thread_cnt;
	for (UINT32 i = 0; i < thread_cnt; ++i) {

		tasks[i].pscene = pscene;
		tasks[i].cnt = tri_cnt_thread;
		tasks[i].ptri_idx = ptri_idx;
		tasks[i].start_idx = tri_cnt_thread*i;
		tasks[i].triBBox = pTriBBox;
		thread_bucket.SetThreadTask(i, &tasks[i]);
	}
	tasks[thread_cnt-1].cnt += (cnt % thread_cnt);
	
	thread_bucket.Run();

	out_bbox.SetEmpty();
	for (UINT32 i = 0; i < thread_cnt; ++i) {
		out_bbox.Add(tasks[i].bbox);
	}
}

float CalcuSplittingPosition(const UINT32* ptri_idx, UINT32 cnt, 
							 const KBBox& bbox, const KBBox* pTriBBox,
							 int splitAxis, std::vector<UINT32>& pigeonHoles)
{
	float startPos = bbox.mMin[splitAxis];
	float endPos = bbox.mMax[splitAxis];
	float face_area[3];
	bbox.GetFaceArea(face_area);
	float bottomRatio = face_area[splitAxis] * 2.0f / (face_area[(splitAxis+1) % 3] + face_area[(splitAxis+2) % 3]);

	if (cnt < 128) {
		float splitRatio = 0.5f;
		float axisLength = endPos - startPos;
		
		return startPos + axisLength * splitRatio;
	}
	else {
		int pigeonHoleCnt = int(sqrt((float)cnt) + 0.5f);
		int startIdx = 0;
		int endIdx = pigeonHoleCnt - 1;
		size_t mem_size = (size_t)pigeonHoleCnt * 2;
		if (pigeonHoles.size() < mem_size)
			pigeonHoles.resize(mem_size);

		float stepLen = (endPos - startPos) / (float)pigeonHoleCnt;
		// Step 1: clear all the pigeon holes
		for (int i = 0; i < pigeonHoleCnt; i++)
			pigeonHoles[i] = 0;
		// Step 2: Calculate and update the min/max bin for each triangle
		for (UINT32 i = 0; i < cnt; ++i) {
			UINT32 idx = (ptri_idx ? ptri_idx[i] : i);
			int binIdx[2];
			for (int j = 0; j < 2; ++j) {
				float tBinIdx = (pTriBBox[idx][j][splitAxis] - startPos) / stepLen;
				if (tBinIdx < 0) 
					tBinIdx = 0;
				else if (tBinIdx >= (float)pigeonHoleCnt)
					tBinIdx = (float)pigeonHoleCnt - 0.5f;

				binIdx[j] = (int)tBinIdx;
			}
			
			++pigeonHoles[binIdx[0]];
			++pigeonHoles[binIdx[1]];
		}

		//------------- Now determine the splitting position by SAH----------
		
		// Step 3: Calculate the left and right accumulated triangle count
		pigeonHoles[pigeonHoleCnt*2 - 1] = 0;
		for (int i = pigeonHoleCnt - 2; i >= 0; --i) 
			pigeonHoles[i + pigeonHoleCnt] = pigeonHoles[i+1] + pigeonHoles[pigeonHoleCnt + i + 1];
		for (int i = 1; i < pigeonHoleCnt; ++i) 
			pigeonHoles[i] += pigeonHoles[i - 1];

		// Step 4: Evaluate the SAH value for each pigeon hole's boundary
		float minSAH = FLT_MAX;
		float splitRatio = 0.5f;
		for (int i = 0; i < pigeonHoleCnt - 1; ++i) {
			UINT32 leftAccum = pigeonHoles[i];
			UINT32 rightAccum = pigeonHoles[pigeonHoleCnt + i];
			assert(leftAccum + rightAccum == cnt*2);
			float tRatio = float(i + 1)/float(pigeonHoleCnt);
			float tSAH = (bottomRatio + tRatio) * float(leftAccum) + 
				(bottomRatio + 1.0f - tRatio) * float(rightAccum);
			if (minSAH > tSAH) {
				minSAH = tSAH;
				splitRatio = tRatio;
			}
		}
		assert(splitRatio >=0 && splitRatio <= 1.0f);
		return startPos + (endPos - startPos) * splitRatio;
	}

}