#include "Entry.h"
#include "Constants.h"
#include "../util/HelperFunc.h"
#include "../camera/camera_manager.h"
#include "../material/material_library.h"
#include <FreeImage.h>

namespace KRayTracer {

static KRayTracer_Root* g_pRoot = NULL;

KRayTracer_Root* InitializeKRayTracer()
{
	FreeImage_Initialise();

	Material_Library::Initialize();
	CameraManager::Initialize();
	LightScheme::Initialize();

	g_pRoot = new KRayTracer_Root();
	return g_pRoot;
}

void DestroyKRayTracer()
{
	LightScheme::Shutdown();
	CameraManager::Shutdown();
	Material_Library::Shutdown();

	if (g_pRoot)
		delete g_pRoot;
	g_pRoot = NULL;
	FreeImage_DeInitialise();
}

KRayTracer_Root::KRayTracer_Root()
{
	mpSceneLoader.reset(new SceneLoader());
}

KRayTracer_Root::~KRayTracer_Root()
{
}

bool KRayTracer_Root::SetConstant(const char* name, const char* value)
{
	if (mpTracingEntry.get()) {
		printf("Cannot change the constants when scene is loaded.\n");
		return false;
	}
	return SetGlobalConstant(name, value);
}

bool KRayTracer_Root::LoadScene(const char* filename)
{
	mpSceneLoader.reset(NULL);
	mpSceneLoader.reset(new KRayTracer::SceneLoader());
	if (mpSceneLoader->LoadFromFile(filename)) {

		return true;
	}
	else
		return false;
}

bool KRayTracer_Root::LoadUpdateFile(const char* filename)
{
	if (mpSceneLoader.get() && mpSceneLoader->LoadUpdatingFile(filename))
		return true;
	else
		return false;
}

bool KRayTracer_Root::SaveScene(const char* filename)
{
	if (mpSceneLoader.get())
		return mpSceneLoader->SaveToFile(filename);
	else
		return false;
}

void KRayTracer_Root::CloseScene()
{
	mpSceneLoader.reset(NULL);
}


bool KRayTracer_Root::Render(UINT32 w, UINT32 h, const char* output_file, double& render_time, double& output_time)
{
	if (mpTracingEntry.get() == NULL)
		mpTracingEntry.reset(new SamplingThreadContainer);

	RenderParam param;
	param.is_refining = false;
	param.want_motion_blur = false;
	param.want_depth_of_field = false;
	param.want_global_illumination = false;
	param.want_edge_sampling = true;
	param.sample_cnt_eval = PIXEL_SAMPLE_CNT_EVAL;
	param.sample_cnt_more = PIXEL_SAMPLE_CNT_MORE;
	param.sample_cnt_edge = PIXEL_SAMPLE_CNT_EDGE;

	param.image_width = w;
	param.image_height = h;

	KTimer stop_watch(true);
		
	bool res = mpTracingEntry->Render(param, &mEventCB, NULL, mpSceneLoader.get());
	render_time = stop_watch.Stop();

	stop_watch.Start();
	std::auto_ptr<BitmapObject> bmp(BitmapObject::CreateBitmap(w, h, BitmapObject::eRGBA8));
	bmp->CopyFromRGB32F(*mpTracingEntry->mRenderBuffers.GetOutputImagePtr(), 0, 0, 0, 0, w, h);
	bmp->Save(output_file);
	output_time = stop_watch.Stop();
	return res;
}

void KRayTracer_Root::SetCamera(const char* name, float pos[3], float lookat[3], float up_vec[3], float xfov)
{
	if (mpTracingEntry.get()) {
		KCamera* pCamera = CameraManager::GetInstance()->GetCameraByName(name);
		if (!pCamera) {
			pCamera = CameraManager::GetInstance()->OpenCamera(name, true);
		}
		
		KVec3 cpos(pos[0], pos[1], pos[2]);
		cpos += mpSceneLoader->mImportSceneOffset;
		cpos *= mpSceneLoader->mImportSceneScale;
		
		KVec3 clookat(lookat[0],lookat[1], lookat[2]);
		clookat += mpSceneLoader->mImportSceneOffset;
		clookat *= mpSceneLoader->mImportSceneScale;
		
		KVec3 cup(up_vec[0], up_vec[1], up_vec[2]);
		KCamera::MotionState ms;
		ms.pos = cpos;
		ms.lookat = clookat;
		ms.up = cup;
		ms.xfov = xfov;
		pCamera->SetupStillCamera(ms);
	}
}

void KRayTracer_Root::EventNotifier::OnTileFinished(UINT32 sx, UINT32 sy, UINT32 w, UINT32 h)
{

}

void KRayTracer_Root::EventNotifier::OnFrameFinished(bool bIsUserCancel)
{
	mFrameFinishMutex.Signal();
}


} // namespace KRayTracer