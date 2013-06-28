#include "AbcLoader.h"

#include <Alembic/AbcGeom/All.h>
#include <Alembic/AbcCoreHDF5/All.h>
namespace Abc = Alembic::Abc;

AbcLoader::AbcLoader()
{

}

AbcLoader::~AbcLoader()
{

}

bool AbcLoader::Load(const char* filename)
{
	Abc::IArchive archive(Alembic::AbcCoreHDF5::ReadArchive(), filename);

	return true;
}