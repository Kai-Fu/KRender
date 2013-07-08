#pragma once

#include "../base/geometry.h"
#include "../scene/KKDTreeScene.h"
#include <string>
#include <list>
#include <hash_map>
#include <Alembic/AbcGeom/All.h>
#include <Alembic/Abc/Foundation.h>
namespace Abc = Alembic::Abc::ALEMBIC_VERSION_NS;
namespace AbcG = Alembic::AbcGeom::ALEMBIC_VERSION_NS;
namespace AbcA = ::Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS;
class KKDBBoxScene;



class AbcLoader
{
public:
	AbcLoader();
	~AbcLoader();

	bool Load(const char* filename, KKDBBoxScene& scene);

private:
	void ProcessNode(const Abc::IObject& obj, int treeDepth = 0);

	// Functions to convert different types of Abc objects.
	// Each of these functions will return true if it needs to process its children, otherwise false is returned.
	void ProcessMesh(const AbcG::IPolyMesh& mesh);
	void ProcessCamera(const AbcG::ICamera& camera);

	void GetXformWorldTransform(const AbcG::IXform& xform, std::vector<KMatrix4>& frames);
	KKDTreeScene* GetXformStaticScene(const Abc::IObject& obj, KMatrix4& mat);

	static void ConvertMatrix(const Imath::M44d& ilmMat, KMatrix4& mat);
private:
	double mCurTime;
	double mSampleDuration;
	int mXformSampleCnt;
	int mDeformSampleCnt;

	KKDBBoxScene* mpScene;
	std::hash_map<AbcA::ObjectReader*, KKDTreeScene*> mXformNodes;

};