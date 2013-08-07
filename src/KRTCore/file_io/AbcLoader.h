#pragma once

#include "../base/geometry.h"
#include "../scene/KKDTreeScene.h"
#include <string>
#include <list>
#include <map>
#include <hash_map>
#include <Alembic/AbcGeom/All.h>
#include <Alembic/Abc/Foundation.h>
namespace Abc = Alembic::Abc::ALEMBIC_VERSION_NS;
namespace AbcG = Alembic::AbcGeom::ALEMBIC_VERSION_NS;
namespace AbcA = ::Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS;
class KSceneSet;



class AbcLoader
{
public:
	AbcLoader();
	~AbcLoader();

	bool Load(const char* filename, KSceneSet& scene);
	bool Update(float time);

public:
	double mAnimStartTime;
	double mAnimEndTime;

private:
	void ProcessNode(const Abc::IObject& obj, int treeDepth = 0);

	// Functions to convert different types of Abc objects.
	// Each of these functions will return true if it needs to process its children, otherwise false is returned.
	void ProcessMesh(const AbcG::IPolyMesh& mesh);
	void ProcessCamera(const AbcG::ICamera& camera);

	void GetObjectWorldTransform(const AbcG::IObject& obj, KMatrix4 trans[2], bool& isAnim);
	KScene* GetXformStaticScene(const Abc::IObject& obj, KMatrix4& mat);

	bool ConvertMesh(const AbcG::IPolyMeshSchema& meshSchema, Abc::chrono_t t, KTriMesh& outMesh);

	static void ConvertMatrix(const Imath::M44d& ilmMat, KMatrix4& mat);

	template <typename ArrayType>
	static bool CompareAbcArrary(const ArrayType& left, const ArrayType& right) {
		if (left->size() != right->size())
			return false;
		for (size_t i = 0; i < left->size(); ++i) {
			if ((*left)[i] != (*right)[i]) return false;
		}
		return true;
	}

	void GetCurNodeID(std::vector<size_t>& nodeId) const;
	void UpdateXformNode(std::vector<size_t>::const_iterator nodeIdIt, std::vector<size_t>::const_iterator nodeItEnd, UINT32 nodeIdx, const Abc::IObject& parentObj);
	void UpdateAnimSubScene(std::vector<size_t>::const_iterator nodeIdIt, std::vector<size_t>::const_iterator nodeItEnd, KScene* pScene, const Abc::IObject& parentObj);

private:
	double mCurTime;
	double mSampleDuration;

	KSceneSet* mpScene;
	std::string mFileName;
	std::hash_map<AbcA::ObjectReader*, KScene*> mXformNodes;

	std::vector<size_t> mCurNodeID;
	int mCurTreeDepth;

	std::map<std::vector<size_t>, UINT32> mAnimNodeIndices;
	std::map<std::vector<size_t>, KScene*> mAnimSubScenes;

};