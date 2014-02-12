#pragma once
#include "../base/base_header.h"
#include "../util/helper_func.h"

#include "tracing_thread_container.h"
#include "scene_loader.h"


namespace KRayTracer {

	class KRayTracer_Root
	{
	public:
		KRayTracer_Root();
		~KRayTracer_Root();

		bool SetConstant(const char* name, const char* value);

		bool LoadScene(const char* filename);
		bool UpdateTime(double timeInSec, double duration);
		void CloseScene();

		const BitmapObject* Render(UINT32 w, UINT32 h, KRT_ImageFormat destFormat, void* pUserBuf, double& render_time);
	
		// Set the basic parameter for a given camera, if the specified camera name doesn't exist, it will be created
		void SetCamera(const char* name, float pos[3], float lookat[3], float up_vec[3], float xfov);

	public:
		std::auto_ptr<KRayTracer::SamplingThreadContainer> mpTracingEntry;
		std::auto_ptr<KRayTracer::SceneLoader> mpSceneLoader;

	private:
		class EventNotifier : public KRayTracer::ImageSampler::EventCallBack
		{
		public:
			virtual void OnTileFinished(UINT32 sx, UINT32 sy, UINT32 w, UINT32 h);
			virtual void OnFrameFinished(bool bIsUserCancel);
			KEvent mFrameFinishMutex;
		};
		EventNotifier mEventCB;
	};

	bool InitializeKRayTracer();
	void DestroyKRayTracer();

	FILE* OpenFile(const char* filename, bool textMode, std::string* out_path = NULL);

} // namespace KRayTracer