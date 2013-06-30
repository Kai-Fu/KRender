#pragma once


class KKDBBoxScene;

class AbcLoader
{
public:
	AbcLoader();
	~AbcLoader();

	bool Load(const char* filename, KKDBBoxScene& scene);
};