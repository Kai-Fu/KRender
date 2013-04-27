#pragma once
#include "../base/BaseHeader.h"
#include <string>

class KSerializerBase
{

public:
	KSerializerBase() {}
	~KSerializerBase() {}

	virtual bool Begin(bool readOrWrite) = 0;
	virtual bool End() = 0;
	virtual bool Write(void* pData, unsigned int size) = 0;
	virtual bool Read(void* pData, unsigned int size) = 0;
};


class KDiskFileSerializer : public KSerializerBase
{
public:
	KDiskFileSerializer();
	~KDiskFileSerializer();
public:
	void SetFilePathName(const char* filename);

	virtual bool Begin(bool readOrWrite);
	virtual bool End();
	virtual bool Write(void* pData, unsigned int size);
	virtual bool Read(void* pData, unsigned int size);

private:
	std::string mFileName;
	FILE* mpFileHandle;
};