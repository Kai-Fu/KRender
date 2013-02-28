#pragma once
#include "../base/BaseHeader.h"
#include "../util/HelperFunc.h"

#include "TracingThreadCntr.h"
#include "SceneLoader.h"


namespace KRayTracer {

	class KRT_API KRayTracer_Root
	{
	public:
		KRayTracer_Root();
		~KRayTracer_Root();

		bool SetConstant(const char* name, const char* value);

		bool LoadScene(const char* filename);
		bool SaveScene(const char* filename);
		bool LoadUpdateFile(const char* filename);
		void CloseScene();

		bool Render(UINT32 w, UINT32 h, const char* output_file, double& render_time, double& output_time);
	
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

	KRT_API KRayTracer_Root* InitializeKRayTracer();
	KRT_API void DestroyKRayTracer();

} // namespace KRayTracer