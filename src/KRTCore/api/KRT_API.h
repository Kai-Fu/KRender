#pragma once
#include <vector>

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
	kRGBA_32F,
	kR_32F
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
typedef void* ShaderHandle;

namespace Geom {
	struct RawMesh;
}

KRT_API bool KRT_SaveUpdate(const char* file_name, 
			const std::vector<unsigned>& modified_cameras,
			const std::vector<unsigned>& modified_lights,
			const std::vector<unsigned>& modified_morph_nodes, 
			const std::vector<unsigned>& modified_RBA_nodes);

extern "C" {

	KRT_API bool KRT_Initialize();
	KRT_API void KRT_Destory();

	KRT_API bool KRT_LoadScene(const char* fileName, KRT_SceneStatistic& statistic);
	KRT_API const char* KRT_GetCurrentScenePath();
	KRT_API bool KRT_UpdateTime(double timeInSec, double duration);

	KRT_API void KRT_CloseScene();
	KRT_API bool KRT_SetConstant(const char* name, const char* value);
	KRT_API bool KRT_SetRenderOption(const char* name, const char* value);


	KRT_API bool KRT_RenderToMemory(unsigned w, unsigned h, KRT_ImageFormat format, void* pOutData, KRT_RenderStatistic& outStatistic);
	KRT_API bool KRT_RenderToImage(unsigned w, unsigned h, KRT_ImageFormat format, const char* fileName, KRT_RenderStatistic& outStatistic);

	KRT_API bool KRT_SetCamera(const char* cameraName, float pos[3], float lookat[3], float up_vec[3], float xfov);
	KRT_API bool KRT_SetActiveCamera(const char* cameraName);
	KRT_API unsigned KRT_GetCameraCount();
	KRT_API const char* KRT_GetCameraName(unsigned idx);

	KRT_API unsigned KRT_AddLightSource(const char* shaderName, float matrix[16], float intensity[3]);
	KRT_API void KRT_DeleteAllLights();

	KRT_API unsigned KRT_AddMeshToSubScene(Geom::RawMesh* pMesh, SubSceneHandle subScene);
	KRT_API void KRT_AddNodeToSubScene(SubSceneHandle subScene, const char* nodeName, unsigned meshIdx, ShaderHandle hShader, float* matrix);

	KRT_API unsigned KRT_AddSubScene(TopSceneHandle scene);
	KRT_API SubSceneHandle KRT_GetSubSceneByIndex(TopSceneHandle scene, unsigned idx);
	KRT_API unsigned KRT_AddNodeToScene(TopSceneHandle scene, unsigned sceneIdx, float* matrix);
	KRT_API void KRT_ResetSubSceneNodeTransform(TopSceneHandle scene, unsigned nodeIdx);

	KRT_API TopSceneHandle KRT_GetScene();
	
	KRT_API ShaderHandle KRT_CreateSurfaceShader(const char* templateFile, const char* mtlName);
	KRT_API ShaderHandle KRT_GetSurfaceShader(const char* mtlName);
	KRT_API bool KRT_SetShaderParameter(ShaderHandle hShader, const char* paramName, void* valueData, unsigned dataSize);

	KRT_API ShaderHandle KRT_GetNodeSurfaceShader(const char* nodeName);
	KRT_API void KRT_SetNodeSurfaceShader(const char* nodeName, ShaderHandle shader);


}