#pragma once

#include "../base/BaseHeader.h"
#include "../base/geometry.h"
#include "../base/image_filter.h"
#include "../intersection/EyeRayGen.h"
#include "../scene/KKDBBoxScene.h"
#include "../shader/light_scheme.h"
#include "../util/tile2d.h"
#include "../camera/camera_manager.h"
#include "../util/thread_model.h"

#include "SceneLoader.h"

#include "../image/BitmapObject.h"
#include "../image/color.h"

#include <vector>
#include <common/defines/stl_inc.h>

namespace KRayTracer {

	class ImageSampler : public ThreadModel::IThreadTask
	{
	public:
		class EventCallBack
		{
		public:
			virtual ~EventCallBack() {}
			// NOTE: all the call backs should be re-entrant
			virtual void OnTileFinished(UINT32 sx, UINT32 sy, UINT32 w, UINT32 h) = 0;
			virtual void OnFrameFinished(bool bIsUserCancel) = 0;
		};

		
		struct InputData {
			SceneLoader*			pScene;
			volatile long			stopSignal;
			KCamera*				pCurrentCamera;
			RenderBuffers*			pRenderBuffers;
			Tile2DSet*				pImageTile2D;
			EventCallBack*			pEventCB;
			const KRBG32F_EdgeDetecter* pEdgeFlag;
		};

		RenderParam*		mpRenderParam;
		InputData*			mpInputData;
		std::vector<KVec2>	mAreaLightUnifiedSamples;
		std::auto_ptr<TracingInstance> mTracingThreadData;

	protected:
		struct PixelSamplingResult {
			KColor average;
			float variance;
			float alpha;
		};
	
	private:
	
		std::vector<KColor>		mTempSamplingRes;
		// Current bounce depth of the ray
		UINT32					mCurBounceDepth;


	public:
		ImageSampler();
		virtual ~ImageSampler();

		virtual void Execute();

		void SetOutputImage(UINT32 totalWidth, UINT32 totalHeight,
			UINT32 sx, UINT32 sy, 
			UINT32 w, UINT32 h, 
			BitmapObject* pBmp);

		void DoPixelSampling(UINT32 x, UINT32 y, UINT32 sample_count, PixelSamplingResult& result);
		bool SampleTile();
		void AccumCurrentPixel(UINT32 x, UINT32 y, UINT32 sample_count, const KColor& clr);
	};

} // namespace KRayTracer
