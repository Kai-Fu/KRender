#pragma once
#include <common/defines/stl_inc.h>
#include <string>
#include "camera.h"


class KRT_API CameraManager
{
public:
	CameraManager();
	~CameraManager();

	UINT32 GetCameraCnt() const;
	KCamera* OpenCamera(const char* name, bool forceCreate);

	const char* GetActiveCamera() const;
	bool SetActiveCamera(const char* name);
	KCamera* GetCameraByName(const char* name);

	void ResetIter();
	const char* GetNextCamera();
	void Clear();

	static CameraManager* GetInstance();
	static void Initialize();
	static void Shutdown();

	bool Save(FILE* pFile);
	bool Load(FILE* pFile);

protected:
	typedef STDEXT::hash_map<std::string, KCamera*> CAMERA_NAME_TO_PTR;
	CAMERA_NAME_TO_PTR mCameras;
	CAMERA_NAME_TO_PTR::iterator mCameraIter;
	std::string mActiveCameraName;

	static CameraManager* s_pInstance;
};