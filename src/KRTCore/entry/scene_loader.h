#pragma once

#include "../base/base_header.h"
#include "../scene/bvh_scene.h"
#include "../file_io/abc_loader.h"

#include <map>

namespace KRayTracer {

	class SceneLoader 
	{
	public:
		SceneLoader();
		~SceneLoader();

		bool LoadFromFile(const char* file_name);
		bool UpdateTime(double timeInSec, double duration);

	private:
		void BuildNodeIdMap();

	public:
		bool mIsFromOBJ;
		bool mIsSceneLoaded;
		AbcLoader mAbcLoader;

		KSceneSet* mpScene;
		KAccelStruct_BVH* mpAccelData;
		double mLoadingTime;
		UINT32 mFileLoadingTime;

		struct NodeId
		{
			UINT32 sceneIdx;
			UINT32 nodeIdx;
		};
		std::map<std::string, NodeId> mNodeIDs;
	};

} // namespace KRayTracer