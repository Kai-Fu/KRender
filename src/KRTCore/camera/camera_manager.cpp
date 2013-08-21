#include "camera_manager.h"
#include "../file_io/file_io_template.h"
#include "../util/HelperFunc.h"


CameraManager* CameraManager::s_pInstance;

CameraManager::CameraManager()
{
}

CameraManager::~CameraManager()
{
	Clear();
}

UINT32 CameraManager::GetCameraCnt() const
{
	return (UINT32)mCameras.size();
}

KCamera* CameraManager::OpenCamera(const char* name, bool forceCreate)
{
	CAMERA_NAME_TO_PTR::iterator it = mCameras.find(name);
	KCamera* pCamera;
	if (it == mCameras.end() && forceCreate) {
		UINT32 i = 0;
		char dstBuf[40];
		std::string newName(name);
		while (it != mCameras.end()) {
			newName = name;
			IntToStr(i, dstBuf, 10);
			newName += dstBuf;
			it = mCameras.find(newName);
			++i;
		}

		pCamera = new KCamera();
		mCameras[newName] = pCamera;
		BuildCameraIndices();
		return pCamera;
	}
	else {
		return it == mCameras.end() ? NULL : it->second;
	}
	
}

const char* CameraManager::GetActiveCamera() const
{
	if (mActiveCameraName.empty()) {
		if (mCameras.empty())
			return NULL;
		else
			return mCameras.begin()->first.c_str();
	}
	else {
		CAMERA_NAME_TO_PTR::const_iterator it = mCameras.find(mActiveCameraName);
		if (it != mCameras.end())
			return it->first.c_str();
		else
			return NULL;
	}
		
}

KCamera* CameraManager::GetCameraByName(const char* name)
{
	CAMERA_NAME_TO_PTR::const_iterator it = mCameras.find(name);
	if (it != mCameras.end())
		return it->second;
	else
		return NULL;
}

KCamera* CameraManager::GetCameraByIndex(UINT32 idx)
{
	if (idx < mCameraArray.size())
		return mCameraArray[idx];
	else
		return NULL;
}

const char* CameraManager::GetCameraNameByIndex(UINT32 idx)
{
	if (idx < mCamNameArray.size())
		return mCamNameArray[idx].c_str();
	else
		return NULL;
}

void CameraManager::BuildCameraIndices()
{
	mCameraArray.clear();
	CAMERA_NAME_TO_PTR::const_iterator it = mCameras.begin();
	for (; it != mCameras.end(); ++it) {
		mCameraArray.push_back(it->second);
		mCamNameArray.push_back(it->first);
	}
}

CameraManager* CameraManager::GetInstance()
{
	return s_pInstance;
}

void CameraManager::Initialize()
{
	s_pInstance = new CameraManager();
}

void CameraManager::Shutdown()
{
	delete s_pInstance;
	s_pInstance = NULL;
}

void CameraManager::Clear()
{
	CAMERA_NAME_TO_PTR::iterator it = mCameras.begin();
	for (; it != mCameras.end(); ++it) {
		delete it->second;
	}
	mCameras.clear();
	mActiveCameraName = "";
}

bool CameraManager::SetActiveCamera(const char* name)
{
	if (name && mCameras.find(name) != mCameras.end()) {
		mActiveCameraName = name;
		return true;
	}
	else
		return false;
}
