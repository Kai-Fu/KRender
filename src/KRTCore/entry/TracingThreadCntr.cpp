#include "TracingThreadCntr.h"
#include "../util/helper_func.h"
#include "../camera/camera_manager.h"
#include "../entry/Constants.h"

namespace KRayTracer {

SamplingThreadContainer::SamplingThreadContainer()
{
	UINT32 threadCnt = GetConfigedThreadCount();
	mpSharedThreadBucket.reset(new ThreadModel::ThreadBucket(threadCnt));
}

SamplingThreadContainer::~SamplingThreadContainer()
{

}


bool SamplingThreadContainer::Render(
			const RenderParam& param, 
			ImageSampler::EventCallBack* pCB, 
			const char* camera_name, 
			SceneLoader* scene)
{
	mRenderParam = param;

	mRenderInputData.pScene = scene;
	mRenderInputData.stopSignal = 0;
	if (camera_name)
		mRenderInputData.pCurrentCamera = CameraManager::GetInstance()->GetCameraByName(camera_name);
	else
		mRenderInputData.pCurrentCamera = CameraManager::GetInstance()->GetCameraByName(CameraManager::GetInstance()->GetActiveCamera());

	if (!mRenderInputData.pCurrentCamera)
		return false;
	// setup the image size of the camera
	mRenderInputData.pCurrentCamera->SetImageSize(param.image_width, param.image_height);

	// allocate the internal buffers for rendering
	mRenderBuffers.SetImageSize(param.image_width, param.image_height, param.pixel_format, param.user_buffer);
	mTile2D.Reset(param.image_width, param.image_height, 32);
	mRenderInputData.pRenderBuffers = &mRenderBuffers;
	mRenderInputData.pImageTile2D = &mTile2D;
	mRenderInputData.pEventCB = pCB;
	

	mImageSamplerThreads.resize(mpSharedThreadBucket->GetThreadCnt());
	for (UINT32 i = 0; i < mImageSamplerThreads.size(); ++i) {
		mImageSamplerThreads[i].mpRenderParam = &mRenderParam;
		mImageSamplerThreads[i].mpInputData = &mRenderInputData;
		mImageSamplerThreads[i].mTracingThreadData.reset(new TracingInstance(mRenderInputData.pScene->mpAccelData, mRenderInputData.pRenderBuffers));
		mpSharedThreadBucket->SetThreadTask(i, &mImageSamplerThreads[i]);
	}

	// Do the first pass of sampling 
	mRenderInputData.pEdgeFlag = NULL;
	mpSharedThreadBucket->Run();

	if (param.sample_cnt_edge > 0 && param.want_edge_sampling) {
		mTile2D.Reset(param.image_width, param.image_height, 32);
		// Perform the edge detection for further sampling
		std::auto_ptr<KRBG32F_EdgeDetecter> pEdgeFlag(new KRBG32F_EdgeDetecter(mRenderBuffers.GetOutputImagePtr(), mpSharedThreadBucket->GetThreadCnt()));
		mRenderInputData.pEdgeFlag = pEdgeFlag.get();
		pEdgeFlag->RunFilter(mpSharedThreadBucket.get());

		for (UINT32 i = 0; i < mImageSamplerThreads.size(); ++i) 
			mpSharedThreadBucket->SetThreadTask(i, &mImageSamplerThreads[i]);
	
		// Do the second pass of sampling
		mpSharedThreadBucket->Run();
	}

	return true;
}


}

