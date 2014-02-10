#pragma once

#include "../base/geometry.h"
#include "thread_model.h"
#include "../scene/kd_tree_scene.h"

void FilterByBBox(UINT32* ptri_idx, UINT32& cnt, KBBox& out_bbox, 
				  ThreadModel::ThreadBucket& thread_bucket, 	
				  const KBBox* pTriBBox,
				  const KAccelStruct_KDTree* pscene, const KBBox* clamp_box);

void CalcuTriangleArrayBBox(const UINT32* ptri_idx, UINT32 cnt, KBBox& out_bbox, 
							ThreadModel::ThreadBucket& thread_bucket, 
							const KBBox* pTriBBox,
							const KAccelStruct_KDTree* pscene);

float CalcuSplittingPosition(const UINT32* ptri_idx, UINT32 cnt, 
							 const KBBox& bbox, const KBBox* pTriBBox,
							 int splitAxis, std::vector<UINT32>& pigeonHoles);