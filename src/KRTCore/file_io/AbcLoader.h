#pragma once


class AbcLoader
{
public:
	AbcLoader();
	~AbcLoader();

	bool Load(const char* filename);
};