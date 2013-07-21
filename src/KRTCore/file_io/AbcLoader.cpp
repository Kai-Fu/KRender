#include "AbcLoader.h"


#include <Alembic/AbcCoreHDF5/All.h>
#include <Alembic/Abc/ISchemaObject.h>
#include <Alembic/AbcGeom/IPolyMesh.h>
#include <Alembic/AbcGeom/ICamera.h>
#include <Alembic/AbcGeom/ILight.h>
#include <Alembic/AbcCoreAbstract/TimeSampling.h>

#include "../scene/KKDBBoxScene.h"
#include <assert.h>


AbcLoader::AbcLoader()
{
	mCurTime = 0;
	mpScene = NULL;
	mSampleDuration = 1.0 / 24.0;
	mXformSampleCnt = 12;
	mDeformSampleCnt = 3;
}

AbcLoader::~AbcLoader()
{

}

bool AbcLoader::Load(const char* filename, KSceneSet& scene)
{
	mpScene = &scene;
	Abc::IArchive archive(Alembic::AbcCoreHDF5::ReadArchive(), filename);

	std::string appName;
	std::string libraryVersionString;
	Alembic::Util::uint32_t libraryVersion;
	std::string whenWritten;
	std::string userDescription;
	Abc::GetArchiveInfo(archive,
				appName,
				libraryVersionString,
				libraryVersion,
				whenWritten,
				userDescription);
	if (appName != "") {
		std::cout << ">>> file written by: " << appName << std::endl;
		std::cout << ">>> using Alembic : " << libraryVersionString << std::endl;
		std::cout << ">>> written on : " << whenWritten << std::endl;
		std::cout << ">>> user description : " << userDescription << std::endl;
		std::cout << std::endl;
	}

	Abc::IObject topObj(archive, Abc::kTop);

	ProcessNode(topObj);
	return true;
}

void AbcLoader::ProcessNode(const Abc::IObject& obj, int treeDepth)
{
	if ( !obj ) { return; }

    // IObject has no explicit time sampling, but its children may.
    size_t numChildren = obj.getNumChildren();
    for (size_t i = 0; i < numChildren; ++i) {
        const Abc::ObjectHeader &ohead = obj.getChildHeader(i);

		for (int s = 0; s < treeDepth; ++s) std::cout << " ";

        if ( AbcG::IPolyMesh::matches(ohead) ) {
            AbcG::IPolyMesh pmesh(obj, ohead.getName());
            if (pmesh) {
				std::cout << "mesh: " << pmesh.getName() << std::endl; 
				ProcessMesh(pmesh);
            }
        }
        else if (AbcG::IPoints::matches(ohead)) {
            AbcG::IPoints points(obj, ohead.getName());
            if (points) {
                std::cout << "points: " << points.getName() << std::endl; 
            }
        }
        else if (AbcG::ICurves::matches(ohead) ) {
            AbcG::ICurves curves(obj, ohead.getName());
            if (curves) {
                std::cout << "curves: " << curves.getName() << std::endl; 
            }
        }
        else if (AbcG::INuPatch::matches(ohead)) {
            AbcG::INuPatch nuPatch(obj, ohead.getName());
            if (nuPatch) {
				std::cout << "nuPatch: " << nuPatch.getName() << std::endl; 
            }
        }
        else if (AbcG::IXform::matches(ohead)) {
            AbcG::IXform xform(obj, ohead.getName());
            if (xform) {
				std::cout << "xform: " << xform.getName() << std::endl;
            }
        }
        else if (AbcG::ISubD::matches(ohead)) {
            AbcG::ISubD subd(obj, ohead.getName());
            if (subd) {
                std::cout << "subd: " << subd.getName() << std::endl; 
            }
        }
		else if (AbcG::ICamera::matches(ohead)) {
			AbcG::ICamera camera(obj, ohead.getName());
			if (camera) {
                std::cout << "camera: " << camera.getName() << std::endl; 
            }
		}
		else if (AbcG::ILight::matches(ohead)) {
			AbcG::ILight light(obj, ohead.getName());
			if (light) {
                std::cout << "light: " << light.getName() << std::endl; 
            }
		}
       
        {
			// Look for the children of current object
			Abc::IObject childObj(obj, ohead.getName());
			ProcessNode(childObj, treeDepth + 1);
        }
    }
}


KScene* AbcLoader::GetXformStaticScene(const Abc::IObject& obj, KMatrix4& mat)
{
	Imath::M44d localMat;
	localMat.makeIdentity();
	AbcA::ObjectReaderPtr animNode;
	for (Abc::IObject node = obj; node.valid(); node = node.getParent()) {

		if (AbcG::IXform::matches(node.getHeader())) {
			AbcG::IXform xform(node, Abc::kWrapExisting);
			
			if (!xform.getSchema().isConstant()) {
				animNode = xform.getPtr();
				break;
			}
			localMat *= xform.getSchema().getValue().getMatrix();
		}
	}


	if (mXformNodes.find(animNode.get()) == mXformNodes.end()) {
		UINT32 sceneIdx;
		KScene* xformScene = mpScene->AddKDScene(sceneIdx);
		mXformNodes[animNode.get()] = xformScene;
		if (animNode.get()) {
			AbcG::IXform xform(animNode, Abc::kWrapExisting);
			std::vector<KMatrix4> sceneNodeFrames;
			GetXformWorldTransform(xform, sceneNodeFrames);
			UINT32 sceneNodeIdx = mpScene->SceneNode_Create(sceneIdx);
			mpScene->SceneNodeTM_ResetFrame(sceneNodeIdx);
			for (size_t i = 0; i < sceneNodeFrames.size(); ++i)
				mpScene->SceneNodeTM_AddFrame(sceneNodeIdx, sceneNodeFrames[i]);
		}
		else {
			// Create the default node with identify transform.
			mpScene->SceneNode_Create(sceneIdx);
		}

	}

	ConvertMatrix(localMat, mat);
	KScene* xformScene = mXformNodes[animNode.get()];
	return xformScene;
}

void AbcLoader::ProcessMesh(const AbcG::IPolyMesh& mesh)
{
	KMatrix4 localMat;
	KScene* pScene = GetXformStaticScene(mesh, localMat);
	assert(pScene);

	UINT32 meshIdx = pScene->AddMesh();
	KTriMesh* pMesh = pScene->GetMesh(meshIdx);
	assert(pMesh);

	UINT32 nodeIdx = pScene->AddNode();
	KNode* pNode = pScene->GetNode(nodeIdx);
	pNode->mMesh.push_back(meshIdx);
	pScene->SetNodeTM(nodeIdx, localMat);
	pNode->mpSurfShader = NULL;

	ConvertStaticMesh(mesh.getSchema(), mCurTime, *pMesh);
	/*if (mesh.getSchema().isConstant()) {
		// static mesh
		AbcG::IPolyMeshSchema::Sample psamp;
		mesh.getSchema().get(psamp);

	}
	else {
		// deformable mesh
		for (int i = 0; i < mDeformSampleCnt; ++i) {

		}
	}*/
}

void AbcLoader::ConvertMatrix(const Imath::M44d& ilmMat, KMatrix4& mat)
{
	for (int i = 0; i < 16; ++i)
		((float*)&mat)[i] = (float)ilmMat.getValue()[i];
}

bool AbcLoader::ConvertStaticMesh(const AbcG::IPolyMeshSchema& meshSchema, Abc::chrono_t t, KTriMesh& outMesh)
{
	AbcG::IPolyMeshSchema::Sample meshSample;
	//Abc::ISampleSelector ss(t, Abc::ISampleSelector::kNearIndex);
	meshSchema.get(meshSample);

	// Get the data of vertex positions and faces
	//
    Abc::P3fArraySamplePtr vertPos = meshSample.getPositions();
    Abc::Int32ArraySamplePtr faceIdx = meshSample.getFaceIndices();
    Abc::Int32ArraySamplePtr faceCnts = meshSample.getFaceCounts();

	// Calculate the triangle count
	size_t triCnt = 0;
	size_t faceCnt = faceCnts->size();
	size_t polyVertCnt = 0;
	for (size_t fi = 0; fi < faceCnt; ++fi) {
		assert((*faceCnts)[fi] > 2);
		triCnt += ((*faceCnts)[fi] - 2);

		polyVertCnt += (*faceCnts)[fi];
	}
	outMesh.mFaces.resize(triCnt);

	//
	// Now for normal data
	//
	AbcG::IN3fGeomParam normParam = meshSchema.getNormalsParam();
	if (normParam.getNumSamples() > 0) { 

		// There is at least one normal sample, so take use of it

		if (normParam.getScope() != Alembic::AbcGeom::kVertexScope &&
			normParam.getScope() != Alembic::AbcGeom::kVaryingScope &&
			normParam.getScope() != Alembic::AbcGeom::kFacevaryingScope ){
			std::cout << "Normal vector has an unsupported scope, skipping mesh: " << meshSchema.getName() << std::endl;
			return false;
		}

		Alembic::AbcGeom::IN3fGeomParam::Sample normSamp;
		normParam.getExpanded(normSamp); // TODO: need to consider the time
		Alembic::Abc::N3fArraySamplePtr normVal = normSamp.getVals();
		size_t normCnt = normVal->size();

		if ((normParam.getScope() == Alembic::AbcGeom::kVertexScope || 
			normParam.getScope() == Alembic::AbcGeom::kVaryingScope) &&
			normCnt == vertPos->size() ) {

			// This mesh has per-vertex normal
				outMesh.SetupPN(normCnt, 1);
			for (size_t i = 0; i < normCnt; ++i) {
				outMesh.GetVertPN(i)->pos[0] = (*vertPos)[i].x;
				outMesh.GetVertPN(i)->pos[1] = (*vertPos)[i].y;
				outMesh.GetVertPN(i)->pos[2] = (*vertPos)[i].z;

				outMesh.GetVertPN(i)->nor[0] = (*normVal)[i].x;
				outMesh.GetVertPN(i)->nor[1] = (*normVal)[i].y;
				outMesh.GetVertPN(i)->nor[2] = (*normVal)[i].z;
			}

			size_t triIter = 0;
			size_t fiIter = 0;
			for (size_t fi = 0; fi < faceCnt; ++fi) {
				for (int vi = 1; vi < (*faceCnts)[fi] - 1; ++vi) {
					outMesh.mFaces[triIter].pn_idx[0] = (*faceIdx)[fiIter];
					outMesh.mFaces[triIter].pn_idx[2] = (*faceIdx)[fiIter + vi];
					outMesh.mFaces[triIter].pn_idx[1] = (*faceIdx)[fiIter + vi + 1];
					++triIter;
				}
				fiIter += (*faceCnts)[fi];
			}

		}
		else if (normCnt == polyVertCnt &&
			normParam.getScope() == Alembic::AbcGeom::kFacevaryingScope) {
			// This mesh has per-face normal
			outMesh.SetupPN(triCnt*3, 1);
			size_t triIter = 0;
			size_t fiIter = 0;
			size_t viIter = 0;
			for (size_t fi = 0; fi < faceCnt; ++fi) {
				for (int vi = 1; vi < (*faceCnts)[fi] - 1; ++vi) {
					int v0 = (*faceIdx)[fiIter];
					int v1  = (*faceIdx)[fiIter + vi];
					int v2  = (*faceIdx)[fiIter + vi + 1];

					int v[] = {v0, v1, v2};
					outMesh.mFaces[triIter].pn_idx[0] = (UINT32)viIter;
					outMesh.mFaces[triIter].pn_idx[2] = (UINT32)viIter + 1;
					outMesh.mFaces[triIter].pn_idx[1] = (UINT32)viIter + 2;
					for (int ii = 0; ii < 3; ++ii) {
						outMesh.GetVertPN(viIter)->pos[0] = (*vertPos)[v[ii]].x;
						outMesh.GetVertPN(viIter)->pos[1] = (*vertPos)[v[ii]].y;
						outMesh.GetVertPN(viIter)->pos[2] = (*vertPos)[v[ii]].z;

						outMesh.GetVertPN(viIter)->nor[0] = (*normVal)[viIter].x;
						outMesh.GetVertPN(viIter)->nor[1] = (*normVal)[viIter].y;
						outMesh.GetVertPN(viIter)->nor[2] = (*normVal)[viIter].z;

						viIter++;
					}
					++triIter;
				}
				fiIter += (*faceCnts)[fi];
			}
		}
	}
	else {

		// There's no normal sample, I need to calcuate the normal from faces and the topology(shared vertices)

		size_t vertCnt = vertPos->size();
		std::vector<unsigned int> sharedCnts(vertCnt);
		std::vector<KVec3> averagedNormals(vertCnt);
		for (size_t i = 0; i < vertCnt; ++i) {
			sharedCnts[i] = 0;
			averagedNormals[i] = KVec3(0,0,0);
		}

		size_t fiIter = 0;
		for (size_t fi = 0; fi < faceCnt; ++fi) {
			assert((*faceCnts)[fi] > 2);
			int v0 = (*faceIdx)[fiIter];
			int v1  = (*faceIdx)[fiIter + 1];
			int v2  = (*faceIdx)[fiIter + 2];

			

			Imath::V3f vert0 = (*vertPos)[v0];
			Imath::V3f vert1 = (*vertPos)[v1];
			Imath::V3f vert2 = (*vertPos)[v2];
			Imath::V3f normTmp = (vert1 - vert0).cross(vert2 - vert0);
			KVec3 norm = KVec3(normTmp.x, normTmp.y, normTmp.z);
			norm.normalize();
			
			for (int vi = 0; vi < (*faceCnts)[fi]; ++vi) {
				size_t vIdx = (*faceIdx)[fiIter + vi];
				averagedNormals[vIdx] += norm;
				sharedCnts[vIdx]++;
			}
			fiIter += (*faceCnts)[fi];
		}

		for (size_t i = 0; i < vertCnt; ++i) {
			if (sharedCnts[i] > 1)
				averagedNormals[i] /= (float)sharedCnts[i];
		}

		// Now fill the data to KTriMesh object
		outMesh.SetupPN(vertCnt, 1);
		for (size_t i = 0; i < vertCnt; ++i) {
			outMesh.GetVertPN(i)->pos[0] = (*vertPos)[i].x;
			outMesh.GetVertPN(i)->pos[1] = (*vertPos)[i].y;
			outMesh.GetVertPN(i)->pos[2] = (*vertPos)[i].z;

			outMesh.GetVertPN(i)->nor = -averagedNormals[i];
		}

		size_t triIter = 0;
		fiIter = 0;
		for (size_t fi = 0; fi < faceCnt; ++fi) {
			for (int vi = 1; vi < (*faceCnts)[fi] - 1; ++vi) {
				outMesh.mFaces[triIter].pn_idx[0] = (*faceIdx)[fiIter];
				outMesh.mFaces[triIter].pn_idx[2] = (*faceIdx)[fiIter + vi];
				outMesh.mFaces[triIter].pn_idx[1] = (*faceIdx)[fiIter + vi + 1];
				++triIter;
			}
			fiIter += (*faceCnts)[fi];
		}
	}

	//
	// Now for UV data
	//
	AbcG::IV2fGeomParam uvParam = meshSchema.getUVsParam();
	if (uvParam.getNumSamples() > 0) {
		Alembic::AbcGeom::IV2fGeomParam::Sample uvSamp;
		uvParam.getIndexed(uvSamp);  // TODO: need to consider the time
		Alembic::AbcGeom::V2fArraySamplePtr uvVal = uvSamp.getVals();
		Alembic::Abc::UInt32ArraySamplePtr uvIdxVal = uvSamp.getIndices();

		size_t uvCnt = uvIdxVal->size();
		if (uvCnt == faceIdx->size()) {
			// per-vertex uv

		}
		else if (uvCnt != vertPos->size() ) {
			// per-polygon per-vertex uv

		}
		else {
			std::cout << " UVs aren't per-vertex or per-polygon per-vertex, skipping mesh: " << meshSchema.getName() << std::endl;
			return false;
		}
	}

	return true;
}

void AbcLoader::GetXformWorldTransform(const AbcG::IXform& xform, std::vector<KMatrix4>& frames)
{
	bool isAniminated = false;
	for (Abc::IObject node = xform; node.valid(); node = node.getParent()) {
		if (AbcG::IXform::matches(node.getHeader())) {
			AbcG::IXform xform(node, Abc::kWrapExisting);
			if (!xform.getSchema().isConstant()) {
				isAniminated = true;
				break;
			}
		}
	}

	if (isAniminated) {

		double sampleStep = mSampleDuration / (double)mXformSampleCnt;
		frames.resize(mXformSampleCnt);
		for (int i = 0; i < mXformSampleCnt; ++i) 
			frames[i] = nvmath::cIdentity44f;


		for (Abc::IObject node = xform; node.valid(); node = node.getParent()) {
			if (AbcG::IXform::matches(node.getHeader())) {
				AbcG::IXform xform(node, Abc::kWrapExisting);

				KMatrix4 localTransform;

				for (int i = 0; i < mXformSampleCnt; ++i) {
					
					Abc::chrono_t sampleTime = mCurTime + sampleStep * (double)i;
					// Do two samples, one with floor index and the other with ceiling index, then
					// lerp between these two samples with the current time.
					Abc::ISampleSelector ss0(sampleTime, Abc::ISampleSelector::kFloorIndex);
					Abc::ISampleSelector ss1(sampleTime, Abc::ISampleSelector::kCeilIndex);
					Abc::TimeSamplingPtr iTsmp = xform.getSchema().getTimeSampling();
					size_t numSamps = xform.getSchema().getNumSamples();
					Abc::chrono_t t0 = iTsmp->getFloorIndex(sampleTime, numSamps).second;
					Abc::chrono_t t1 = iTsmp->getCeilIndex(sampleTime, numSamps).second;
				
					Imath::M44d mat0, mat1;
					mat0 = xform.getSchema().getValue(ss0).getMatrix();
					mat1 = xform.getSchema().getValue(ss1).getMatrix();

					if (sampleTime >= t1) {
						ConvertMatrix(mat1, localTransform);
					}
					else if (sampleTime <= t0) {
						ConvertMatrix(mat0, localTransform);
					}
					else {
						KMatrix4 kt0, kt1;
						ConvertMatrix(mat0, kt0);
						ConvertMatrix(mat1, kt1);
						nvmath::lerp( float((sampleTime-t0)/(t1-t0)), kt0, kt1, localTransform);
					}

					frames[i] = localTransform * frames[i];
				}
			}
		}
	}
	else {
		// For static xform...
		frames.resize(1);
		frames[0] = nvmath::cIdentity44f;
		for (Abc::IObject node = xform; node.valid(); node = node.getParent()) {
			if (AbcG::IXform::matches(node.getHeader())) {
				AbcG::IXform xform(node, Abc::kWrapExisting);
				Imath::M44d mat = xform.getSchema().getValue().getMatrix();
				KMatrix4 localTransform;
				ConvertMatrix(mat, localTransform);
				frames[0] = localTransform * frames[0];
			}

		}
	}

	
}