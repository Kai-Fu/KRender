#pragma once

#ifdef _WIN32
	// Windows platform
	#ifdef BUILD_KRT_CORE_DLL
		#define KRT_API _declspec( dllexport )
	#else
		#define KRT_API _declspec( dllimport )
	#endif

	#pragma warning( disable: 4251 )
#else
	// Non-windows platform, e.g. Linux
	# define DLL_PUBLIC	__attribute__ ((visibility("default")))
	# define DLL_PRIVATE __attribute__ ((visibility("hidden")))
	typedef unsigned long long UINT64;
	#ifdef BUILD_KRT_CORE_DLL
		#define KRT_API DLL_PUBLIC
	#else
		#define KRT_API DLL_PUBLIC
	#endif

#endif // _WIN32

	
enum KRT_ImageFormat
{
	kRGB_8,
	kRGBA_8,
	kRGB_32F,
	kRGBA_32F
};

struct KRT_SceneStatistic
{
	unsigned kd_build_time;
	unsigned gen_accel_geom_time;
	unsigned kd_finialize_time;
	unsigned long long actual_triangle_count;
	unsigned long long leaf_triangle_count;
	unsigned long long kd_leaf_count;
	unsigned long long kd_node_count;
	unsigned bbox_leaf_count;
	unsigned bbox_node_count;
};

struct KRT_RenderStatistic
{
	double render_time;

};

typedef void* SubSceneHandle;
typedef void* TopSceneHandle;

namespace Geom {
	struct RawMesh;
}

extern "C" {

	KRT_API bool KRT_Initialize();
	KRT_API void KRT_Destory();

	KRT_API bool KRT_LoadScene(const char* fileName, KRT_SceneStatistic& statistic);
	KRT_API bool KRT_LoadUpdate(const char* fileName);
	KRT_API bool KRT_SaveScene(const char* fileName);
	KRT_API void KRT_CloseScene();
	KRT_API bool KRT_SetConstant(const char* name, const char* value);
	KRT_API bool KRT_SetRenderOption(const char* name, const char* value);


	KRT_API bool KRT_RenderToMemory(unsigned w, unsigned h, KRT_ImageFormat format, void* pOutData, KRT_RenderStatistic& outStatistic);
	KRT_API bool KRT_RenderToImage(unsigned w, unsigned h, KRT_ImageFormat format, const char* fileName, KRT_RenderStatistic& outStatistic);

	KRT_API bool KRT_SetCamera(const char* cameraName, float pos[3], float lookat[3], float up_vec[3], float xfov);
	KRT_API bool KRT_SetActiveCamera(const char* cameraName);
	KRT_API unsigned KRT_GetCameraCount();
	KRT_API const char* KRT_GetCameraName(unsigned idx);

	KRT_API unsigned KRT_AddLightSource(float pos[3], float xyz_rot[3]);
	KRT_API void KRT_DeleteAllLights();

	KRT_API unsigned KRT_AddMeshToSubScene(Geom::RawMesh* pMesh, SubSceneHandle subScene);

}