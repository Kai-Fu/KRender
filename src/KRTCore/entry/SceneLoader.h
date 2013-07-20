#pragma once

#include "../base/BaseHeader.h"
#include "../scene/KKDBBoxScene.h"
#include <map>

namespace KRayTracer {

	class SceneLoader 
	{
	public:
		SceneLoader();
		~SceneLoader();
		// Save/load the scene and tracking camera data
		bool SaveToFile(const char* file_name) const;
		bool SaveUpdatingFile(const char* file_name, 
			const std::vector<UINT32>& modified_cameras,
			const std::vector<UINT32>& modified_lights, 
			const std::vector<UINT32>& modified_morph_nodes, 
			const std::vector<UINT32>& modified_RBA_nodes) const;

		bool LoadFromFile(const char* file_name);
		bool LoadUpdatingFile(const char* file_name);

	private:
		bool SaveAsSCN(FILE* pFile) const;
		bool LoadAsSCN(FILE* pFile);

		void BuildNodeIdMap();

	public:
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