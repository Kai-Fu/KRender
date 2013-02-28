#include <fstream>
#include <iostream>
using namespace std;
#include "KObjFileLoader.h"
#include "../util/HelperFunc.h"
#include "../base/raw_geometry.h"

#pragma warning( disable : 4996)

KObjFileLoader::KObjFileLoader(void)
{
	mUseTexMap = true;
	mAdjustObjCenter = true;
}

KObjFileLoader::~KObjFileLoader(void)
{

}


bool KObjFileLoader::LoadObjFile(const char* filename, KScene& scene, KVec3& scene_offset, float& scene_scale)
{
	GLMmodel* model = glmReadOBJ(filename);
	if (!model)	return false;

	float* pImportVertex = model->vertices + 3;
	float* pImportNormal = model->normals + 3;
	float* pImportTexcrd = model->texcoords + 2;

	if (!model->has_smooth_group && model->numnormals == 0) {
		// No normal data and no smooth group, I need to create normal for this case
		glmVertexNormals(model, 90.0f);
	}

	
	KBBox bbox;
	for (UINT32 i = 0; i < model->numvertices; ++i) {
		bbox.ContainVert(KVec3(pImportVertex[i*3], pImportVertex[i*3+1], pImportVertex[i*3+2]));
	}
	if (mAdjustObjCenter) {
		// Adjust the scale of the imported model
		KVec3 c = bbox.Center();
		scene_offset = -c;
		float scale = 10.0f / nvmath::length(bbox.mMax - bbox.mMin);
		scene_scale = scale;
		for (UINT32 i = 0; i < model->numvertices; ++i) {
			pImportVertex[i*3] += scene_offset[0];
			pImportVertex[i*3+1] += scene_offset[1];
			pImportVertex[i*3+2] += scene_offset[2];
			
			pImportVertex[i*3] *= scale;
			pImportVertex[i*3+1] *= scale;
			pImportVertex[i*3+2] *= scale;
		}
		bbox.mMin = KVec3(-5.0f, -5.0f, -5.0f);
		bbox.mMax = KVec3(5.0f, 5.0f, 5.0f);
	}
	mObjBBox = bbox;

	Geom::RawMesh* pMesh = new Geom::RawMesh();
	{
		std::vector<UINT32> smGroups(model->numtriangles);

		UINT32 elemCnt = 2;
		UINT32 uvCnt = model->numtexcoords;
		pMesh->mFacePosIdx.resize(model->numtriangles);
		pMesh->mFaceNorIdx.resize(model->numtriangles);
		if (uvCnt > 0) {
			pMesh->mTexData.resize(uvCnt);
			memcpy(&pMesh->mTexData[0], pImportTexcrd, sizeof(KVec2)*uvCnt);
			pMesh->mFaceTexIdx.resize(model->numtriangles);
		}
		pMesh->mPosData.resize(model->numvertices);
		memcpy(&pMesh->mPosData[0], pImportVertex, sizeof(float)*3*model->numvertices);

		if (model->numnormals > 0) {
			pMesh->mNorData.resize(model->numnormals);
			memcpy(&pMesh->mNorData[0], pImportNormal, sizeof(float)*3*model->numnormals);
		}
		
		UINT32 validFaceCnt = 0;		

		for (UINT32 face_i = 0; face_i < model->numtriangles; ++face_i) {

			UINT32 idx0, idx1, idx2;
			if (uvCnt > 0) {
				idx0 = model->triangles[face_i].tindices[0] ;
				idx1 = model->triangles[face_i].tindices[1];
				idx2 = model->triangles[face_i].tindices[2];
				if (idx0 > model->numtexcoords || idx1 > model->numtexcoords || idx2 > model->numtexcoords ||
					idx0 == 0 || idx1 == 0 || idx2 == 0 ||
					(idx0 == idx1 && idx1 == idx2)) {
						pMesh->mFaceTexIdx[validFaceCnt].idx[0] = INVALID_INDEX;
						pMesh->mFaceTexIdx[validFaceCnt].idx[1] = INVALID_INDEX;
						pMesh->mFaceTexIdx[validFaceCnt].idx[2] = INVALID_INDEX;
				}
				else {
					pMesh->mFaceTexIdx[validFaceCnt].idx[0] = idx0-1;
					pMesh->mFaceTexIdx[validFaceCnt].idx[0] = idx1-1;
					pMesh->mFaceTexIdx[validFaceCnt].idx[0] = idx2-1;
				}
			}

			int ii = 0;
			for (; ii < 3; ++ii) {
				UINT32 idx0 = model->triangles[face_i].vindices[ii] - 1;
				if (idx0 >= model->numvertices) 
					break;
				pMesh->mFacePosIdx[validFaceCnt].idx[ii] = idx0;
				
				if (model->numnormals > 0) {
					idx1 = model->triangles[face_i].nindices[ii] - 1;
					if (idx1 >= model->numnormals)
						break;
					
					pMesh->mFaceNorIdx[validFaceCnt].idx[ii] = idx1;
				}
			}
			if (ii != 3)
				continue;

			smGroups[validFaceCnt] = model->triangles[face_i].sm_group;
			++validFaceCnt;
		}

		pMesh->SetFaceCnt(validFaceCnt, model->numtexcoords > 0);
		smGroups.resize(validFaceCnt);

		if (model->numnormals == 0) {
			// No normal? just build it regarding the smooth groups
			pMesh->BuildNormals(smGroups);
		}
		
	}
	
	
	stdext::hash_map<UINT32, UINT32> node2Mtl;
	// Create scene node and separate the meshes
	GLMgroup* pGroup = model->groups;
	while (pGroup) {

		stdext::hash_map<UINT32, std::vector<UINT32> > mtlIdx2Faces;
		for (UINT32 i = 0; i < pGroup->numtriangles; ++i) {
			UINT32 triIdx = pGroup->triangles[i];
			mtlIdx2Faces[model->triangles[triIdx].mindex].push_back(triIdx);
		}

		stdext::hash_map<UINT32, std::vector<UINT32> >::iterator it = mtlIdx2Faces.begin();
		int mtlIdx = 0;
		for (; it != mtlIdx2Faces.end(); ++it) {
			// Create geometry data
			UINT32 mesh_idx = scene.AddMesh();
			UINT32 node_idx = scene.AddNode();
			KNode* pNode = scene.GetNode(node_idx);
			KTriMesh* pCurMesh = scene.GetMesh(mesh_idx);
			pNode->mMesh.push_back(mesh_idx);
			pNode->mName = pGroup->name;
			if (mtlIdx > 0) {
				char mtlStr[40];
				IntToStr(mtlIdx, mtlStr, 10);
				pNode->mName += "[";
				pNode->mName += mtlStr;
				pNode->mName += "]";
			}
			node2Mtl[node_idx] = it->first;
			Geom::RawMesh* tempMesh = new Geom::RawMesh();
			pMesh->Detach(it->second, *tempMesh);
			Geom::CompileOptimizedMesh(*tempMesh, *pCurMesh);
			delete tempMesh;

			KMatrix4 mat;
			GetNodeLocalMatrix(pGroup->name, pCurMesh, mat);
			pCurMesh->ChangePivot(mat);
			scene.SetNodeTM(node_idx, mat);

			++mtlIdx;
		}
		// move to next group
		pGroup = pGroup->next;
	}

	ProcessingMaterial(model, scene, node2Mtl);
	// Free the temporary memory allocated for this obj file processing
	glmDelete(model);
	delete pMesh;
	return true;
}

void KObjFileLoader::GetNodeLocalMatrix(const char* nodeName, KTriMesh* pMesh, KMatrix4& outMat)
{
	KBBox bbox;
	pMesh->ComputeBBox(bbox);
	KMatrix4 mat;
	nvmath::setTransMat(mat, bbox.Center());
	outMat = mat;
}