#include "FbxNodesToKScene.h"
#include <set>
#include <assert.h>
#include <KRTCore/base/raw_geometry.h>

#include <common/math/Matnnt.cpp>

static FbxManager* mySdkManager = NULL;
static FbxImporter* myImporter = NULL;
static FbxScene* myScene = NULL;
static std::map<FbxObject*, std::pair<FbxMesh*, int> > myObjectToMesh;
static FbxLongLong myRBAnimInteropCnt = 8;

/************************************************************************/
// Utility function to convert the materials assigned on the node into ISurfaceShader.
/************************************************************************/
void GetConvertedMaterials(FbxNode* pFbxNode, std::vector<ShaderHandle>& convertedMtls);


/************************************************************************/
// Initialize the FBX SDK and related objects for the specified FBX file.
/************************************************************************/
int InitFbxSDK(const char* filename)
{
	mySdkManager = FbxManager::Create();
	if (!mySdkManager) {
		printf("Failed to initialize FBX SDK.\n");
		return -1;
	}

	{
		// print the FBX SDK version info
		int major, minor, rev;
		mySdkManager->GetFileFormatVersion(major, minor, rev);
		printf("FBXSDK version info: %d.%d.%d\n", major, minor, rev);
	}

	// Load the file
	myImporter = FbxImporter::Create(mySdkManager, "");
	const bool lImportStatus =
		myImporter->Initialize(filename, -1, mySdkManager->GetIOSettings());
	if(!lImportStatus) {
		printf("Failed to load the FBX file.\n");
		return -1;
	}

	{
		// Print the file version
		int major, minor, rev;
		myImporter->GetFileVersion(major, minor, rev);
		printf("Imported file version info: %d.%d.%d\n", major, minor, rev);
	}

	{
		// Now handle the import
		FbxIOSettings * ios = FbxIOSettings::Create(mySdkManager, IOSROOT );
		mySdkManager->SetIOSettings(ios);
		mySdkManager->GetIOSettings()->SetBoolProp(IMP_FBX_MATERIAL, true);
		mySdkManager->GetIOSettings()->SetBoolProp(IMP_FBX_TEXTURE, true);
		mySdkManager->GetIOSettings()->SetBoolProp(IMP_FBX_LINK, false);
		mySdkManager->GetIOSettings()->SetBoolProp(IMP_FBX_SHAPE, false);
		mySdkManager->GetIOSettings()->SetBoolProp(IMP_FBX_GOBO, false);
		mySdkManager->GetIOSettings()->SetBoolProp(IMP_FBX_ANIMATION, true);
		mySdkManager->GetIOSettings()->SetBoolProp(IMP_FBX_GLOBAL_SETTINGS, true);

		myScene = FbxScene::Create(mySdkManager,"");
		if (!myImporter->Import(myScene)) {
			printf("Failed to imported scene graph.\n");
			return -1;
		}

		// Assume Z-up world
		FbxAxisSystem axisSystem(FbxAxisSystem::eMayaZUp);
		axisSystem.ConvertScene(myScene);
	}

	return 1;
}

/************************************************************************/
// Destroy all the stuff before quiting
/************************************************************************/
void DestroyFbxSDK()
{
	myImporter->Destroy();
	mySdkManager->Destroy();
}


/************************************************************************/
// Tells if the node has rigid body animation regarding its local LRS.
/************************************************************************/
bool hasLocalRigidBodyAnim(FbxNode* node) 
{
	FbxAnimCurveNode* curveNode[3] = { NULL, NULL, NULL };

	curveNode[0] = node->LclTranslation.GetCurveNode();
	curveNode[1] = node->LclRotation.GetCurveNode();
	curveNode[2] = node->LclScaling.GetCurveNode();
	
	for (int i = 0; i < 3; ++i) {
		if (!curveNode[i]) continue;

		FbxTimeSpan timeSpan;
		curveNode[i]->GetAnimationInterval(timeSpan);
		if (timeSpan.GetStop() != FBXSDK_TIME_INFINITE)
			return true;
			
	}

	return false;
}

/************************************************************************/
// Recursively triangulate all the meshes under the specified node, 
// now only the rigid body animation is supported, so I don't need to 
// consider the deformable meshes.
/************************************************************************/
static void TriangulateMeshes(FbxNode* root_node)
{
	if (!root_node) return;


	FbxObject* pObject = root_node->GetNodeAttribute();

	if (pObject) {
		FbxNodeAttribute::EType type = root_node->GetNodeAttribute()->GetAttributeType();

		if (myObjectToMesh.find(pObject) == myObjectToMesh.end()) {
			FbxGeometryConverter converter(mySdkManager);
			FbxMesh* pMesh = NULL;
			switch(type) {
			case FbxNodeAttribute::eMesh:
				{
					pMesh = FbxCast<FbxMesh>(pObject);
					if (pMesh && !pMesh->IsTriangleMesh()) 
						pMesh = converter.TriangulateMesh(pMesh);
				}
				break;
			case FbxNodeAttribute::eNurbs:
				{
					FbxNurbs* pNurb = dynamic_cast<FbxNurbs*>(pObject);
					if (pNurb)
						pMesh = converter.TriangulateNurbs(pNurb);
				}
				break;
			case FbxNodeAttribute::ePatch:
				{
					FbxPatch* pPatch = dynamic_cast<FbxPatch*>(pObject);
					if (pPatch)
						pMesh = converter.TriangulatePatch(pPatch);
				}
				break;
			}

			if (pMesh) {
				std::pair<FbxMesh*, int> meshRef(pMesh, 1);
				myObjectToMesh[pObject] = meshRef;
			}
		}
		else {
			// The mesh was used by another node, so it's instanced.
			myObjectToMesh[pObject].second++;
		}
	}

	for (int i = 0; i < root_node->GetChildCount(); ++i) {
		TriangulateMeshes(root_node->GetChild(i));
	}

}

void TriangulateScene()
{
	TriangulateMeshes(myScene->GetRootNode());
}


void FilterNodes(Filtered_Nodes& out_filtered_nodes, FbxNode* node, int instanced_geom_tri_limit, bool first_level)
{
	FbxNodeAttribute::EType type = FbxNodeAttribute::eUnknown;
	if (!first_level && hasLocalRigidBodyAnim(node)) {

		out_filtered_nodes.anim_nodes_level0.push_back(node);
	}
	else {

		if (node->GetNodeAttribute()) 
			type = node->GetNodeAttribute()->GetAttributeType();
		switch (type) {
		case FbxNodeAttribute::eMesh:
		case FbxNodeAttribute::eNurbs:
		case FbxNodeAttribute::ePatch:
			{
				FbxObject* pObject = node->GetNodeAttribute();
				std::map<FbxObject*, std::pair<FbxMesh*,int> >::iterator it = myObjectToMesh.find(pObject);
				if (it != myObjectToMesh.end()) {
					// For static part of the scene, I only track the node with geometry data
					FbxMesh* pMesh = it->second.first;
					if (it->second.second == 1 ||
						pMesh->GetPolygonCount() < instanced_geom_tri_limit)
						out_filtered_nodes.static_nodes.push_back(node);
					else 
						out_filtered_nodes.instanced_static_nodes.push_back(node);
				}
			}
			break;
		case FbxNodeAttribute::eLight:
			out_filtered_nodes.light_nodes.push_back(node);
			break;

		case FbxNodeAttribute::eCamera:
			out_filtered_nodes.camera_nodes.push_back(node);
			break;
		}


		for (int i = 0; i < node->GetChildCount(); ++i)
			FilterNodes(out_filtered_nodes, node->GetChild(i), instanced_geom_tri_limit, false);
	}
}


int FbxMeshToKUVMesh(FbxMesh* pInMesh, SubSceneHandle scene, std::vector<int>& out_mesh_idx, std::vector<int>& out_mesh_matId)
{
	FbxLayerElementNormal* fbxNormalLayer = NULL;
	FbxLayerElementUV* fbxTexcrdLayer = NULL;
	FbxLayerElementMaterial* fbxMaterialLayer = NULL; // For face material ID

	int tri_cnt = pInMesh->GetPolygonCount();
	int vert_cnt = pInMesh->GetControlPointsCount();
	if (tri_cnt == 0 || vert_cnt == 0)
		return -1;

	fbxNormalLayer = pInMesh->GetLayer(0)->GetNormals();
	fbxTexcrdLayer = pInMesh->GetLayer(0)->GetUVs();

	// Get the material IDs for each triangle
	for (int l = 0; l < pInMesh->GetLayerCount(); l++) {
		// Find the first material layer
		fbxMaterialLayer = pInMesh->GetLayer(l)->GetMaterials();
		if (fbxMaterialLayer)
			break;
	}

	int singleMatId = 0;
	std::map<int, std::vector<UINT32> > matIDset;
	if (fbxMaterialLayer) {
		if (FbxLayerElement::eAllSame == fbxMaterialLayer->GetMappingMode()) {

		}
		else {
			assert(tri_cnt == fbxMaterialLayer->GetIndexArray().GetCount());
			for (int i = 0; i < tri_cnt; ++i) {
				int matID = fbxMaterialLayer->GetIndexArray().GetAt(i);
				matIDset[matID].push_back((UINT32)i);
			}

			// Make sure there are at least two material IDs in this array
			if (matIDset.size() <= 1) {
				singleMatId = matIDset.begin()->first;
				matIDset.clear();
			}
		}
	}

	Geom::RawMesh* pRawMesh = new Geom::RawMesh();
	int texcrd_cnt = fbxTexcrdLayer ? fbxTexcrdLayer->GetDirectArray().GetCount() : 0;
	// First get the vertex data
	pRawMesh->SetFaceCnt(tri_cnt, texcrd_cnt > 0);
	int* pVertIdx = pInMesh->GetPolygonVertices();
	for (int fi = 0; fi < tri_cnt; ++fi) {
		for (int v = 0; v < 3; ++v)
			pRawMesh->mFacePosIdx[fi].idx[v] = pVertIdx[fi*3 + v];
	}
	FbxVector4* pVert = pInMesh->GetControlPoints();
	pRawMesh->mPosData.resize(vert_cnt);
	for (int i = 0; i < vert_cnt; ++i) {
		pRawMesh->mPosData[i][0] = (float)pVert[i][0];
		pRawMesh->mPosData[i][1] = (float)pVert[i][1];
		pRawMesh->mPosData[i][2] = (float)pVert[i][2];
	}


	{
		// For normals
		int normal_cnt = fbxNormalLayer->GetDirectArray().GetCount();
		pRawMesh->mNorData.resize(normal_cnt);
		for (int i = 0; i < normal_cnt; ++i) {
			FbxVector4 norm = fbxNormalLayer->GetDirectArray().GetAt(i);
			pRawMesh->mNorData[i][0] = (float)norm[0];
			pRawMesh->mNorData[i][1] = (float)norm[1];
			pRawMesh->mNorData[i][2] = (float)norm[2];
		}
		switch (fbxNormalLayer->GetReferenceMode()) {
		case FbxLayerElement::eDirect:
			assert(normal_cnt % 3 == 0);
			for (int fi = 0; fi < tri_cnt; ++fi) {
				for (int v = 0; v < 3; ++v)
					pRawMesh->mFaceNorIdx[fi].idx[v] = fi * 3 + v;
			}
			break;
		case FbxLayerElement::eByControlPoint:
			for (int fi = 0; fi < tri_cnt; ++fi) {
				for (int v = 0; v < 3; ++v)
					pRawMesh->mFaceNorIdx[fi].idx[v] = pInMesh->GetPolygonVertex(fi, v);
			}
			break;
		case FbxLayerElement::eByPolygonVertex:
			{
				int norm_idx_cnt = fbxNormalLayer->GetIndexArray().GetCount();
				assert(norm_idx_cnt == tri_cnt*3);
				for (int i = 0; i < norm_idx_cnt; ++i)
					pRawMesh->mFaceNorIdx[i / 3].idx[i % 3] = fbxNormalLayer->GetIndexArray().GetAt(i);
			}
			break;
		default:
			return -1;
		}
	}

	if (fbxTexcrdLayer)	{
		// For texture coordinates
		pRawMesh->mTexData.resize(texcrd_cnt);
		pRawMesh->mFaceTexIdx.resize(tri_cnt);
		for (int i = 0; i < texcrd_cnt; ++i) {
			FbxVector2 uv = fbxTexcrdLayer->GetDirectArray().GetAt(i);
			pRawMesh->mTexData[i][0] = (float)uv[0];
			pRawMesh->mTexData[i][1] = (float)uv[1];
		}
		switch (fbxTexcrdLayer->GetReferenceMode()) {
		case FbxLayerElement::eDirect:
			for (int fi = 0; fi < tri_cnt; ++fi) {
				for (int v = 0; v < 3; ++v)
					pRawMesh->mFaceTexIdx[fi].idx[v] = fi*3 + v;
			}
			break;
		case FbxLayerElement::eByControlPoint:
			for (int fi = 0; fi < tri_cnt; ++fi) {
				for (int v = 0; v < 3; ++v)
					pRawMesh->mFaceTexIdx[fi].idx[v] = pInMesh->GetPolygonVertex(fi, v);
			}
			break;
		case FbxLayerElement::eByPolygonVertex:
			{
				int uv_idx_cnt = fbxTexcrdLayer->GetIndexArray().GetCount();
				assert(uv_idx_cnt == tri_cnt*3);
				for (int i = 0; i < uv_idx_cnt; ++i)
					pRawMesh->mFaceTexIdx[i / 3].idx[i % 3] = fbxTexcrdLayer->GetIndexArray().GetAt(i);
			}
			break;
		default:
			return -1;
		}
	}

	if (!matIDset.empty()) {
		std::map<int, std::vector<UINT32> >::iterator it = matIDset.begin();
		for (; it != matIDset.end(); ++it) {
			Geom::RawMesh* pTempRawMesh = new Geom::RawMesh();
			pRawMesh->Detach(it->second, *pTempRawMesh);
			UINT32 meshIdx = KRT_AddMeshToSubScene(pTempRawMesh, scene);
			delete pTempRawMesh;
			out_mesh_idx.push_back((int)meshIdx);
			out_mesh_matId.push_back(it->first);
		}
		return (int)matIDset.size();
	}
	else {
		UINT32 meshIdx = KRT_AddMeshToSubScene(pRawMesh, scene);
		out_mesh_idx.push_back((int)meshIdx);
		out_mesh_matId.push_back(singleMatId);
		return 1;
	}
}

struct TagMeshInfo
{
	std::vector<int> meshIdx;
	std::vector<int> matIdx;
};


/************************************************************************/
// Utility function to perform tne matrix conversion.
/************************************************************************/
static void MatrixConvert(FbxAMatrix& src, KMatrix4& dst)
{
	for (int i = 0; i < 4; ++i)
		for (int j = 0; j < 4; ++j)
			dst[i][j] = (float)src.GetColumn(j)[i];
}


bool BuildStaticGeometry(std::list<FbxNode*>& static_nodes, FbxNode* pRootNode, SubSceneHandle static_scene)
{
	std::vector<int> out_mesh_idx;
	std::vector<int> out_mesh_matId;
	std::map<FbxObject*, TagMeshInfo> static_meshes;

	FbxAnimEvaluator* pSceneEvaluator = myScene->GetEvaluator();
	FbxAMatrix rootNodeTM_I;
	rootNodeTM_I.SetIdentity();
	if (pRootNode) {
		FbxAMatrix rootNodeTM = pSceneEvaluator->GetNodeGlobalTransform(pRootNode);
		rootNodeTM_I = rootNodeTM.Inverse();
	}

	std::list<FbxNode*>::iterator node_it = static_nodes.begin();
	for (; node_it != static_nodes.end(); ++node_it) {

		FbxObject* pObject = (*node_it)->GetNodeAttribute();

		if (static_meshes.find(pObject) == static_meshes.end()) { 

			TagMeshInfo& meshInfo = static_meshes[pObject];
			printf("[Mesh object] %s ... ", (*node_it)->GetName());
			int res = FbxMeshToKUVMesh(myObjectToMesh[pObject].first, static_scene, meshInfo.meshIdx, meshInfo.matIdx);
			if (res == -1) {
				printf("failed.\n");
				static_meshes.erase(pObject);
				continue;
			}
			printf("done!\n");
		}
		
		assert(static_meshes.find(pObject) != static_meshes.end());
		TagMeshInfo& meshInfo = static_meshes[pObject];
		// Get node's default TRS properties as a transformation matrix
		FbxAMatrix nodeFbxTM = pSceneEvaluator->GetNodeGlobalTransform(*node_it) * rootNodeTM_I;
		KMatrix4 nodeTM;
		MatrixConvert(nodeFbxTM, nodeTM);
		std::vector<ShaderHandle> convertedMtls;
		GetConvertedMaterials(*node_it, convertedMtls);

		for (size_t i = 0; i < meshInfo.meshIdx.size(); ++i) {

			ShaderHandle nodeShader = NULL;
			if (meshInfo.matIdx[i] < (int)convertedMtls.size())
				nodeShader = convertedMtls[meshInfo.matIdx[i]];
			else
				nodeShader = NULL;
			KRT_AddNodeToSubScene(static_scene, (*node_it)->GetName(), (UINT32)meshInfo.meshIdx[i], nodeShader, (float*)&nodeTM);
		}

	}
	return true;
}


void GetConvertedMaterials(FbxNode* pFbxNode, std::vector<ShaderHandle>& convertedMtls)
{
	int matCnt = pFbxNode->GetMaterialCount();
	// Force it to have at least one default material that is retrieved from node color.
	if (matCnt == 0) matCnt = 1;

	for (int m_i = 0; m_i < matCnt; m_i++){
		FbxSurfacePhong *pMtl = FbxCast<FbxSurfacePhong>(pFbxNode->GetMaterial(m_i));

		//go through all the possible textures
		if(pMtl){
			
			unsigned int uID = (unsigned int)pMtl->GetUniqueID();
			std::string mtlName = pMtl->GetName();
			mtlName += "-";
			char buffer[65];
			mtlName += _itoa_s(uID, buffer, 10);

			ShaderHandle pSurfShader = KRT_GetSurfaceMaterial(mtlName.c_str());
			if (pSurfShader) {
				convertedMtls.push_back(pSurfShader);
				continue;
			}

		}
		
		ShaderHandle pPhongSurf = NULL;

		if (pMtl) {
			pPhongSurf = KRT_CreateSurfaceMaterial("simple_phone_mtl_0.template", pMtl->GetName());
			float tmpClr[3];
			float tmpScale;

			tmpScale = (float)pMtl->DiffuseFactor.Get();
			tmpClr[0] = (float)pMtl->Diffuse.Get()[0] * tmpScale;
			tmpClr[1] = (float)pMtl->Diffuse.Get()[1] * tmpScale;
			tmpClr[2] = (float)pMtl->Diffuse.Get()[2] * tmpScale;
			KRT_SetShaderParameter(pPhongSurf, "diffuse_color", tmpClr, sizeof(tmpClr));

			tmpScale = (float)pMtl->SpecularFactor.Get();
			tmpClr[0] = (float)pMtl->Specular.Get()[0] * tmpScale;
			tmpClr[1] = (float)pMtl->Specular.Get()[1] * tmpScale;
			tmpClr[2] = (float)pMtl->Specular.Get()[2] * tmpScale;
			KRT_SetShaderParameter(pPhongSurf, "specular_color", tmpClr, sizeof(tmpClr));

			tmpScale = (float)pMtl->TransparencyFactor.Get();
			tmpClr[0] = (float)pMtl->TransparentColor.Get()[0] * tmpScale;
			tmpClr[1] = (float)pMtl->TransparentColor.Get()[1] * tmpScale;
			tmpClr[2] = (float)pMtl->TransparentColor.Get()[2] * tmpScale;
			KRT_SetShaderParameter(pPhongSurf, "opacity", tmpClr, sizeof(tmpClr));

			tmpScale = (float)pMtl->Shininess.Get();
			KRT_SetShaderParameter(pPhongSurf, "power", &tmpScale, sizeof(float));
			//Diffuse Textures
			FbxProperty lProperty = pMtl->FindProperty(FbxSurfaceMaterial::sDiffuse);
			if(lProperty.IsValid())
			{
				//no layered texture simply get on the property
				int lNbTextures = lProperty.GetSrcObjectCount<FbxTexture>();
				for(int j =0; j<lNbTextures; ++j)
				{

					FbxFileTexture* lTexture = FbxCast <FbxFileTexture> (lProperty.GetSrcObject<FbxTexture>(j));
					if(lTexture)
					{
						KRT_SetShaderParameter(pPhongSurf, "diffuse_map", (void*)lTexture->GetFileName(), 0);
					}
				}
			}
		}
		else {
			std::string wire_color_mtl_name = "__wire_color_";
			wire_color_mtl_name += pFbxNode->GetName();
			pPhongSurf = KRT_CreateSurfaceMaterial("simple_phone_mtl_0.template", wire_color_mtl_name.c_str());

			FbxNodeAttribute* lNodeAttribute = pFbxNode->GetNodeAttribute();
			float tmpClr[3];

			tmpClr[0] = (float)lNodeAttribute->Color.Get()[0];
			tmpClr[1] = (float)lNodeAttribute->Color.Get()[1];
			tmpClr[2] = (float)lNodeAttribute->Color.Get()[2];
			KRT_SetShaderParameter(pPhongSurf, "diffuse_color", tmpClr, sizeof(tmpClr));
			KRT_SetShaderParameter(pPhongSurf, "specular_color", tmpClr, sizeof(tmpClr));
		}

		convertedMtls.push_back(pPhongSurf);
	} 

}

void BuildLights(const std::list<FbxNode*>& light_nodes)
{
	std::list<FbxNode*>::const_iterator it = light_nodes.begin();
	FbxAnimEvaluator* pSceneEvaluator = myScene->GetEvaluator();
	for (; it != light_nodes.end(); ++it) {
		FbxLight* pFbxLight = FbxCast<FbxLight>((*it)->GetNodeAttribute());
		if (pFbxLight) {
			FbxAMatrix& pNodeDefaultGlobalTransform = pSceneEvaluator->GetNodeGlobalTransform(*it);
			KMatrix4 nodeTM;
			MatrixConvert(pNodeDefaultGlobalTransform, nodeTM);

			float intensity[3];
			float intenScale = (float)pFbxLight->Intensity.Get() / 100.0f;
			intensity[0] = float(pFbxLight->Color.Get()[0]) * intenScale;
			intensity[1] = float(pFbxLight->Color.Get()[1]) * intenScale;
			intensity[2] = float(pFbxLight->Color.Get()[2]) * intenScale;

			KRT_AddLightSource("DummyLightShader", (float*)&nodeTM, intensity);
		}
	}
}

/************************************************************************/
// Function to compile the camera, the method is suggested by FBX SDK.
/************************************************************************/
void BuildCameras(const std::list<FbxNode*>& camera_nodes)
{
	std::list<FbxNode*>::const_iterator it = camera_nodes.begin();
	FbxAnimEvaluator* pSceneEvaluator = myScene->GetEvaluator();

	for (; it != camera_nodes.end(); ++it) {
		FbxCamera* pFbxCamera = FbxCast<FbxCamera>((*it)->GetNodeAttribute());

		if (pFbxCamera) {
			FbxAMatrix& pNodeDefaultGlobalTransform = pSceneEvaluator->GetNodeGlobalTransform(*it);

			KVec3 pos_trans;
			FbxVector4 cameraPos = pNodeDefaultGlobalTransform.GetT();
			pos_trans[0] = float(cameraPos[0]);
			pos_trans[1] = float(cameraPos[1]);
			pos_trans[2] = float(cameraPos[2]);
			
			KVec3 lookat_trans;
			FbxNode* targetNode = (*it)->GetTarget();
			FbxVector4 lookAt;
			if (targetNode) {
				lookAt = pSceneEvaluator->GetNodeGlobalTransform(targetNode).GetT();
			}
			else {
				FbxAMatrix cameraRot;
				cameraRot.SetR(pNodeDefaultGlobalTransform.GetR());
				lookAt = cameraRot.MultT(FbxVector4(1, 0, 0)) + cameraPos;
				
			}
			lookat_trans[0] = (float)lookAt[0];
			lookat_trans[1] = (float)lookAt[1];
			lookat_trans[2] = (float)lookAt[2];	

			FbxVector4 upVec;
			FbxNode* upNode = (*it)->GetTargetUp();
			if (upNode) 
				upVec = pSceneEvaluator->GetNodeGlobalTransform(upNode).GetT() - cameraPos;
			else if (targetNode) 
				upVec = pFbxCamera->UpVector.Get();
			else {
				FbxAMatrix cameraRot;
				cameraRot.SetR(pNodeDefaultGlobalTransform.GetR());
				upVec = cameraRot.MultT(FbxVector4(0, 1, 0));
			}
			
			KVec3 up_trans;
			up_trans[0] = (float)upVec[0];
			up_trans[1] = (float)upVec[1];
			up_trans[2] = (float)upVec[2];	
			up_trans.normalize();

			KRT_SetCamera((*it)->GetName(), (float*)&pos_trans, (float*)&lookat_trans, (float*)&up_trans, (float)pFbxCamera->FieldOfViewX.Get());
		}
	}
}


/************************************************************************/
// Compile the FBX scene into KKDBBoxScene
/************************************************************************/
void CompileNodeIntoScene(FbxNode* node, TopSceneHandle bbox_scene, std::list<FbxNode*>& out_anim_nodes) 
{
	Filtered_Nodes filtered_nodes;
	FilterNodes(filtered_nodes, node, INSTANCED_GEOM_LIMIT);

	UINT32 newSceneIdx = KRT_AddSubScene(bbox_scene);
	SubSceneHandle static_scene = KRT_GetSubSceneByIndex(bbox_scene, newSceneIdx);
	KRT_AddNodeToScene(bbox_scene, newSceneIdx, (float*)&nvmath::cIdentity44f);
	BuildStaticGeometry(filtered_nodes.static_nodes, NULL, static_scene);
	BuildStaticInstanceGeometry(filtered_nodes.instanced_static_nodes, bbox_scene);
	BuildLights(filtered_nodes.light_nodes);
	BuildCameras(filtered_nodes.camera_nodes);

	out_anim_nodes.insert(out_anim_nodes.end(), filtered_nodes.anim_nodes_level0.begin(), filtered_nodes.anim_nodes_level0.end());
}


bool BuildStaticInstanceGeometry(std::list<FbxNode*>& instanced_nodes, TopSceneHandle bbox_scene)
{
	return true;
}

bool GatherAnimatedGeometry(FbxNode* animated_root, std::list<FbxNode*>& out_local_static_nodes, std::list<FbxNode*>& out_animated_nodes_next_pass)
{
	// Only need to consider the nodes that have meshes assigned
	FbxObject* pObject = animated_root->GetNodeAttribute();
	if (myObjectToMesh.find(pObject) != myObjectToMesh.end())
			out_local_static_nodes.push_back(animated_root);

	for (int i = 0; i < animated_root->GetChildCount(); ++i) {

		FbxNode* pChild = animated_root->GetChild(i);
		if (hasLocalRigidBodyAnim(pChild))
			out_animated_nodes_next_pass.push_back(pChild);
		else {
			GatherAnimatedGeometry(pChild, out_local_static_nodes, out_animated_nodes_next_pass);
		}
	}

	return !out_animated_nodes_next_pass.empty();
}

bool ProccessAnimatedGeometry(std::list<FbxNode*>& animated_nodes, TopSceneHandle bbox_scene, INDEX_TO_FBXNODE& out_scnIdx_To_fbxNode)
{
	std::list<FbxNode*> out_local_static_nodes = animated_nodes;
	std::list<FbxNode*> animated_nodes_cur_pass = animated_nodes;
	std::list<FbxNode*> animated_nodes_next_pass;
	out_scnIdx_To_fbxNode.clear();
	bool needContinue = false;
	do {

		animated_nodes_next_pass.clear();

		for (std::list<FbxNode*>::iterator it = animated_nodes_cur_pass.begin(); it != animated_nodes_cur_pass.end(); ++it) {

			out_local_static_nodes.clear();
			GatherAnimatedGeometry(*it, out_local_static_nodes, animated_nodes_next_pass);

			if (!out_local_static_nodes.empty()) {
				// Now compile the nodes in to scene
				UINT32 newSceneIdx = KRT_AddSubScene(bbox_scene);
				SubSceneHandle static_scene = KRT_GetSubSceneByIndex(bbox_scene, newSceneIdx);
				FbxAnimEvaluator* pSceneEvaluator = myScene->GetEvaluator();
				FbxAMatrix rootNodeTM = pSceneEvaluator->GetNodeGlobalTransform(*it);
				KMatrix4 nodeTM;
				MatrixConvert(rootNodeTM, nodeTM);
				UINT32 nodeIdx = KRT_AddNodeToScene(bbox_scene, newSceneIdx, (float*)&nodeTM);
				BuildStaticGeometry(out_local_static_nodes, *it, static_scene);
				out_scnIdx_To_fbxNode[nodeIdx] = *it;
			}
		}

		needContinue = !animated_nodes_next_pass.empty();
		animated_nodes_cur_pass.swap(animated_nodes_next_pass);
	} while (needContinue);

	return true;
}

int ProccessAnimatedNodes(const INDEX_TO_FBXNODE& camera_nodes, const INDEX_TO_FBXNODE& light_nodes, const INDEX_TO_FBXNODE& animated_nodes, const char* output_file_base)
{
	FbxTime::EMode time_mode = myScene->GetGlobalSettings().GetTimeMode();
	double fps = FbxTime::GetFrameRate(time_mode);
	FbxTime time_one_frame = FbxTime::GetOneFrameValue(time_mode);
	FbxAnimEvaluator* pSceneEvaluator = myScene->GetEvaluator();
	TopSceneHandle pBBoxScene = KRT_GetScene();
	std::vector<UINT32> modified_cameras;
	std::vector<UINT32> modified_lights;
	std::vector<UINT32> modified_morph_nodes;
	std::vector<UINT32> modified_RBA_nodes;

	modified_RBA_nodes.clear();
	FbxTime time_step = time_one_frame / myRBAnimInteropCnt;
	FbxTime time_start = FBXSDK_TIME_INFINITE;
	FbxTime time_end = FBXSDK_TIME_MINUS_INFINITE;


	// First pass - detemine the total animation interval
	for (INDEX_TO_FBXNODE::const_iterator it = animated_nodes.begin(); it != animated_nodes.end(); ++it) {
		FbxNode* node = it->second;
		FbxAnimCurveNode* curveNode[3] = { NULL, NULL, NULL };
		curveNode[0] = node->LclTranslation.GetCurveNode();
		curveNode[1] = node->LclRotation.GetCurveNode();
		curveNode[2] = node->LclScaling.GetCurveNode();
		for (int i = 0; i < 3; ++i) {
			if (!curveNode[i]) continue;
			FbxTimeSpan timeSpan;
			curveNode[i]->GetAnimationInterval(timeSpan);
			if (time_start > timeSpan.GetStart()) time_start = timeSpan.GetStart();
			if (time_end < timeSpan.GetStop()) time_end = timeSpan.GetStop();
		}
	}


	// Second pass - retrieve the animation data
	int frame_count = 0;
	printf("Proccessing frame 00000");
	for (FbxTime cur_time = time_start; cur_time < time_end; cur_time += time_one_frame) {
		modified_cameras.clear();
		modified_lights.clear();
		modified_morph_nodes.clear();
		modified_RBA_nodes.clear();

		for (INDEX_TO_FBXNODE::const_iterator it = animated_nodes.begin(); it != animated_nodes.end(); ++it) {
			UINT32 idx = it->first;
			FbxNode* node = it->second;
			KRT_ResetSubSceneNodeTransform(pBBoxScene, idx);
			for (int i = 0; i < myRBAnimInteropCnt; ++i) {
				FbxAMatrix animNodeTM = pSceneEvaluator->GetNodeGlobalTransform(node, cur_time + time_step * i);
				KMatrix4 nodeTM;
				MatrixConvert(animNodeTM, nodeTM);
				KRT_AddSubSceneNodeFrame(pBBoxScene, idx, (float*)&nodeTM);
			}
			modified_RBA_nodes.push_back(idx);
		}

		// Finished one frame...
		char frame_str[20];
		printf("\b\b\b\b\b%05d", frame_count);
		sprintf_s(frame_str, ".frame%05d", frame_count);
		std::string file_name = output_file_base;
		file_name += frame_str;

		KRT_SaveUpdate(file_name.c_str(), 
				modified_cameras,
				modified_lights,
				modified_morph_nodes,
				modified_RBA_nodes);
		
		++frame_count;
	}
	printf("\n");
	return frame_count;
}


int ProccessFBXFile(const char* output_file)
{
	// First of all, triangulate the geometry meshes
	TriangulateScene();
	
	TopSceneHandle pBBoxScene = KRT_GetScene();
	std::list<FbxNode*> anim_node_list;
	// Do the first pass of node filtering so that I know which part of the scene is animated.
	CompileNodeIntoScene(myScene->GetRootNode(), pBBoxScene, anim_node_list);

	// For the animated part, use the incremental updating mechanism
	INDEX_TO_FBXNODE rigid_body_anim_nodes;
	ProccessAnimatedGeometry(anim_node_list, pBBoxScene, rigid_body_anim_nodes);
	

	// TODO: handle these kinds of animated nodes
	INDEX_TO_FBXNODE animated_lights;
	INDEX_TO_FBXNODE animated_cameras;

	// Now save the result of FBX processing
	if (!KRT_SaveScene(output_file))
		return -1;

	// Handle and save the animation data
	return ProccessAnimatedNodes(animated_cameras, animated_lights, rigid_body_anim_nodes, output_file);
}