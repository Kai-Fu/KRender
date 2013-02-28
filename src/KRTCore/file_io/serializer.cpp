#include "serializer.h"

KDiskFileSerializer::KDiskFileSerializer()
{
	mpFileHandle = NULL;
}

KDiskFileSerializer::~KDiskFileSerializer()
{
	if (mpFileHandle)
		fclose(mpFileHandle);
}

void KDiskFileSerializer::SetFilePathName(const char* filename)
{
	mFileName = filename;
}

bool KDiskFileSerializer::Begin(bool readOrWrite)
{
	return true;
}

bool KDiskFileSerializer::End()
{
	return true;
}

bool KDiskFileSerializer::Write(void* pData, unsigned int size)
{
	return true;
}

bool KDiskFileSerializer::Read(void* pData, unsigned int size)
{
	return true;
}
