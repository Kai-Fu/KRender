#include "AbcLoader.h"

#include <Alembic/AbcGeom/All.h>
#include <Alembic/AbcCoreHDF5/All.h>
namespace Abc = Alembic::Abc;

#include "../scene/KKDBBoxScene.h"

AbcLoader::AbcLoader()
{

}

AbcLoader::~AbcLoader()
{

}

bool AbcLoader::Load(const char* filename, KKDBBoxScene& scene)
{
	Abc::IArchive archive(Alembic::AbcCoreHDF5::ReadArchive(), filename);
	Abc::IObject topObj(archive, Abc::kTop);

	for (size_t i = 0; i < topObj.getNumChildren(); ++i) {
		Abc::IObject childObj = topObj.getChild(i);
	}

	Alembic::AbcGeom::IGeomBaseObject geomBase(topObj, "meshy");

	Alembic::AbcGeom::IPolyMesh meshyObj(topObj, "meshy");
    Alembic::AbcGeom::IPolyMeshSchema &mesh = meshyObj.getSchema();
	return true;
}