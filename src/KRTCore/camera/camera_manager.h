#pragma once
#include <common/defines/stl_inc.h>
#include <string>
#include "camera.h"


class CameraManager
{
public:
	CameraManager();
	~CameraManager();

	UINT32 GetCameraCnt() const;
	KCamera* OpenCamera(const char* name, bool forceCreate);
	void Clear();

	const char* GetActiveCamera() const;
	bool SetActiveCamera(const char* name);
	KCamera* GetCameraByName(const char* name);
	KCamera* GetCameraByIndex(UINT32 idx);
	const char* GetCameraNameByIndex(UINT32 idx);

	static CameraManager* GetInstance();
	static void Initialize();
	static void Shutdown();

	bool Save(FILE* pFile);
	bool Load(FILE* pFile);

private:
	void BuildCameraIndices();

protected:
	typedef STDEXT::hash_map<std::string, KCamera*> CAMERA_NAME_TO_PTR;
	CAMERA_NAME_TO_PTR mCameras;
	std::vector<KCamera*> mCameraArray;
	std::vector<std::string> mCamNameArray;
	std::string mActiveCameraName;

	static CameraManager* s_pInstance;
};