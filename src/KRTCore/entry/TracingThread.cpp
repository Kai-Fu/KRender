#include "TracingThread.h"
#include "../util/HelperFunc.h"
#include "../intersection/intersect_ray_tri.h"
#include "../intersection/intersect_ray_bbox.h"
#include "../shader/surface_shader.h"
#include <assert.h>


namespace KRayTracer {


ImageSampler::ImageSampler() : 
	mTempSamplingRes(16)
{

}

ImageSampler::~ImageSampler()
{

}

void ImageSampler::DoPixelSampling(UINT32 x, UINT32 y, UINT32 sample_count, PixelSamplingResult& result)
{
	IntersectContext tempCtx;
	if (mTempSamplingRes.size() < sample_count)
		mTempSamplingRes.resize(sample_count);
	float sampleCnt = (float)sample_count;
	float hitCnt = 0;
	KColor sum(0,0,0);
	RenderBuffers* pRBufs = mpInputData->pRenderBuffers;

	for (UINT32 si = 0; si < sample_count; ++si) {

		TracingInstance& tracingInst = *mTracingThreadData.get();
		tracingInst.mCameraContext.inScreenPos = pRBufs->RS_Image(x, y);
		tracingInst.mCameraContext.inMotionTime = ENABLE_MB ? pRBufs->RS_MotionBlur(x, y) : 0;
		tracingInst.mCameraContext.inAperturePos = ENABLE_DOF ? pRBufs->RS_DOF(x, y, mpInputData->pCurrentCamera->GetApertureSize()) : KVec2(0,0);

		KColor out_clr;
		bool isHit = mpInputData->pCurrentCamera->EvaluateShading(tracingInst, mTempSamplingRes[si]);
		sum.Add(mTempSamplingRes[si]);

		mpInputData->pRenderBuffers->IncreaseSampledCount(x, y, 1);

		if (isHit)
			hitCnt += 1.0f;
	}

	result.alpha = hitCnt / sampleCnt;
	result.variance = 0;
	result.average = sum;
	result.average.Scale(1.0f / sampleCnt);

	for (UINT32 si = 0; si < sample_count; ++si) {
		result.variance += mTempSamplingRes[si].DiffRatio(result.average);
	}
	
	result.variance /= sampleCnt;
}

bool ImageSampler::SampleTile()
{
	UINT32 line_width;
	UINT32 out_w, out_h;
	Tile2DSet::TileDesc tileDesc;
	if (!mpInputData->pImageTile2D->GetNextTile(tileDesc))
		return false;  // Finished with all the tile sampling

	line_width = mpRenderParam->image_width;
	out_w = tileDesc.tile_w;
	out_h = tileDesc.tile_h;
	
	UINT32 offset_pass0 = tileDesc.start_y * line_width + tileDesc.start_x;
		
	for (UINT32 y = 0; y < out_h; ++y) {
	UINT32 line_start = y * line_width + offset_pass0;
	for (UINT32 x = 0; x < out_w; ++x) {
		IntersectContext ctxDest;
		// Cast the ray into the scene
		PixelSamplingResult res;
		UINT32 curX = tileDesc.start_x + x;
		UINT32 curY = tileDesc.start_y + y;

		bool isEdgeSampling = false;
		if (mpInputData->pEdgeFlag) {
			// when edge flag is set, only pixels on the edge will get sampled.
			if (!mpInputData->pEdgeFlag->IsEdge(curX, curY)) 
				continue;

			isEdgeSampling = true;
		}


		if (isEdgeSampling) {
			DoPixelSampling(curX, curY, mpRenderParam->sample_cnt_edge, res);
			//AccumCurrentPixel(curX, curY, mpRenderParam->sample_cnt_edge, res.average);
			mpInputData->pRenderBuffers->AddSamples(curX, curY, mpRenderParam->sample_cnt_edge, res.average, res.alpha);
		}
		else {
			DoPixelSampling(curX, curY, mpRenderParam->sample_cnt_eval, res);
			//AccumCurrentPixel(curX, curY, mpRenderParam->sample_cnt_eval, res.average);
			mpInputData->pRenderBuffers->AddSamples(curX, curY, mpRenderParam->sample_cnt_eval, res.average, res.alpha);

			if (mpRenderParam->sample_cnt_eval > 1 && res.variance > COLOR_DIFF_THRESH_HOLD) {
				// Only do the extra sampling when the evaluation sample count is > 1 AND the previous sampling variance is above the threshold
				DoPixelSampling(curX, curY, mpRenderParam->sample_cnt_more, res);
				//AccumCurrentPixel(curX, curY, mpRenderParam->sample_cnt_more, res.average);
				mpInputData->pRenderBuffers->AddSamples(curX, curY, mpRenderParam->sample_cnt_more, res.average, res.alpha);
			}
		}


		if (mpInputData->stopSignal)
			break;
	}
	}

	if (mpInputData->stopSignal)
		return false;

	return true;
}

void ImageSampler::Execute()
{
	while (SampleTile());
}


}

