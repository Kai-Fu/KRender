#pragma once
#include "TracingThread.h"
#include "SceneLoader.h"
#include "../image/bitmap_object.h"

namespace KRayTracer {

	

	class SamplingThreadContainer
	{
	public:
		SamplingThreadContainer();
		virtual ~SamplingThreadContainer();

		bool Render(
			const RenderParam& param, 
			ImageSampler::EventCallBack* pCB, 
			const char* camera_name, 
			SceneLoader* scene);

		// Input & output buffers that need external access
		RenderBuffers mRenderBuffers;
	protected:
				

	protected:
		// Shared thread bucket to perform the tasks in RenderTaskQueue
		std::auto_ptr<ThreadModel::ThreadBucket> mpSharedThreadBucket;

		RenderParam	mRenderParam;
		ImageSampler::InputData		mRenderInputData;
		Tile2DSet					mTile2D;
		std::vector<ImageSampler> mImageSamplerThreads;
		
	};

	


}

