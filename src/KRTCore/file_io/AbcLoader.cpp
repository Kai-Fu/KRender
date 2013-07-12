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

bool AbcLoader::Load(const char* filename, KKDBBoxScene& scene)
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


KKDTreeScene* AbcLoader::GetXformStaticScene(const Abc::IObject& obj, KMatrix4& mat)
{
	Imath::M44d localMat;
	localMat.makeIdentity();
	AbcA::ObjectReader* animNode = NULL;
	for (Abc::IObject node = obj; node.valid(); node = node.getParent()) {

		if (AbcG::IXform::matches(node.getHeader())) {
			AbcG::IXform xform(node, Abc::kWrapExisting);
			
			if (!xform.getSchema().isConstant()) {
				AbcA::ObjectReaderPtr ptr = xform.getPtr();
				animNode = ptr.get();
				break;
			}
			localMat *= xform.getSchema().getValue().getMatrix();
		}
	}


	if (mXformNodes.find(animNode) == mXformNodes.end()) {
		UINT32 sceneIdx;
		KKDTreeScene* xformScene = mpScene->AddKDScene(sceneIdx);
		mXformNodes[animNode] = xformScene;
		if (animNode) {
			AbcG::IXform xform(AbcA::ObjectReaderPtr(animNode), Abc::kWrapExisting);
			std::vector<KMatrix4> sceneNodeFrames;
			GetXformWorldTransform(xform, sceneNodeFrames);
			UINT32 sceneNodeIdx = mpScene->SceneNode_Create(sceneIdx);
			mpScene->SceneNodeTM_ResetFrame(sceneNodeIdx);
			for (size_t i = 0; i < sceneNodeFrames.size(); ++i)
				mpScene->SceneNodeTM_AddFrame(sceneNodeIdx, sceneNodeFrames[i]);
		}

	}

	for (int i = 0; i < 16; ++i)
		((float*)&mat)[i] = (float)localMat.getValue()[i];
	KKDTreeScene* xformScene = mXformNodes[animNode];
	return xformScene;
}

void AbcLoader::ProcessMesh(const AbcG::IPolyMesh& mesh)
{
	KMatrix4 localMat;
	KKDTreeScene* pScene = GetXformStaticScene(mesh, localMat);
	assert(pScene);

	KTriMesh* pMesh = pScene->GetMesh(pScene->AddMesh());

	if (mesh.getSchema().isConstant()) {
		// static mesh
		AbcG::IPolyMeshSchema::Sample psamp;
		mesh.getSchema().get(psamp);

	}
	else {
		// deformable mesh
		for (int i = 0; i < mDeformSampleCnt; ++i) {

		}
	}
}

void AbcLoader::ConvertMatrix(const Imath::M44d& ilmMat, KMatrix4& mat)
{
	for (int i = 0; i < 16; ++i)
		((float*)&mat)[i] = (float)ilmMat.getValue()[i];
}

void AbcLoader::ConvertStaticMesh(const AbcG::IPolyMeshSchema& meshSchema, Abc::chrono_t t, KTriMesh& outMesh)
{
	AbcG::IPolyMeshSchema::Sample meshSample;
	//Abc::ISampleSelector ss(t, Abc::ISampleSelector::kNearIndex);
	meshSchema.get(meshSample);
	// Get arrays of mesh data
    Abc::P3fArraySamplePtr vertPos = meshSample.getPositions();
    Abc::Int32ArraySamplePtr faces = meshSample.getFaceIndices();
    Abc::Int32ArraySamplePtr faceCnt = meshSample.getFaceCounts();
	AbcG::IN3fGeomParam normParam = meshSchema.getNormalsParam();
	
	normParam.getArrayExtent();
	AbcG::IN3fGeomParam::Sample::samp_ptr_type pNormData;
	normParam.getValueProperty().get(pNormData);
	Abc::N3fArraySample* pNorm = pNormData.get();
	pNorm->size();
	Imath::V3f n = (*pNorm)[0];
	const Imath::V3f* pData = pNorm->get();
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