#pragma once

#include <stdio.h>
#define FBXSDK_NEW_API
#include <fbxsdk.h>
#include <list>
#include <map>
#include <KRTCore/api/KRT_API.h>
#include <common/defines/typedefs.h>

#define INSTANCED_GEOM_LIMIT 60

int InitFbxSDK(const char* filename);
void DestroyFbxSDK();

struct KFbxExtMeshRef
{
	FbxMesh* pMesh;
	int ref_cnt;
};


struct Filtered_Nodes 
{
	std::list<FbxNode*> static_nodes;
	// TODO: FBX plug-in for 3ds Max doesn't support instanced geometry
	std::list<FbxNode*> instanced_static_nodes;
	std::list<FbxNode*> anim_nodes_level0;
	std::list<FbxNode*> camera_nodes;
	std::list<FbxNode*> light_nodes;

	void Clear() {
		static_nodes.clear();
		instanced_static_nodes.clear();
		anim_nodes_level0.clear();
		light_nodes.clear();
	}
};

struct Animated_Nodes_Handling_Pass
{
	std::list<FbxNode*> anim_nodes;
};

typedef std::map<UINT32, FbxNode*> INDEX_TO_FBXNODE;
/**
  Filter the nodes in the scene and category them into the types described in Filtered_Nodes.
  The argument instanced_geom_tri_limit is to determine whether a node is needed to be treated as instanced geometry node.
**/
void FilterNodes(Filtered_Nodes& out_filtered_nodes, FbxNode* node, int instanced_geom_tri_limit, bool first_level = true);

/**
  Build the static nodes specified by the input parameter static_nodes, the scene
  info will be stored in the input KKDTreeScene.
**/
bool BuildStaticGeometry(std::list<FbxNode*>& static_nodes, FbxNode* pRootNode, SubSceneHandle static_scene);

/**
  Build the static instanced nodes specified by the input parameter instanced_nodes, the scene
  info will be stored in the input KAccelStruct_BVH.
**/
bool BuildStaticInstanceGeometry(std::list<FbxNode*>& instanced_nodes, TopSceneHandle bbox_scene);

bool GatherAnimatedGeometry(FbxNode* animated_root, std::list<FbxNode*>& out_local_static_nodes, std::list<FbxNode*>& out_animated_nodes_next_pass);
bool ProccessAnimatedGeometry(std::list<FbxNode*>& animated_nodes, TopSceneHandle bbox_scene);

/**
  Proccessing the animated part of the scene.
  The returned value is the frame count.
**/
int ProccessAnimatedNodes(const INDEX_TO_FBXNODE& camera_nodes, const INDEX_TO_FBXNODE& light_nodes, const INDEX_TO_FBXNODE& animated_nodes, const char* output_file_base);
/**
  Input is a FbxMesh, the outputs are the converted mesh index and material index into the input scene.
**/
int FbxMeshToKUVMesh(FbxMesh* pInMesh, SubSceneHandle scene, std::vector<int>& out_mesh_idx, std::vector<int>& out_mesh_matId);


int ProccessFBXFile(const char* output_file);