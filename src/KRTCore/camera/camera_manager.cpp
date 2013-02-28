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

void CameraManager::ResetIter()
{
	mCameraIter = mCameras.begin();
}

const char* CameraManager::GetNextCamera()
{
	if (mCameraIter != mCameras.end()) {
		const char* name = mCameraIter->first.c_str();
		++mCameraIter;
		return name;
	}
	else
		return NULL;
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

bool CameraManager::Save(FILE* pFile)
{
	std::string typeName = "CameraManager";
	if (!SaveArrayToFile(typeName, pFile))
		return false;

	UINT64 cnt = mCameras.size();
	if (!SaveTypeToFile(cnt, pFile))
		return false;

	CAMERA_NAME_TO_PTR::iterator it = mCameras.begin();
	for (; it != mCameras.end(); ++it) {

		if (!SaveArrayToFile(it->first, pFile))
			return false;

		if (!it->second->Save(pFile))
			return false;
	}
	return true;
}

bool CameraManager::Load(FILE* pFile)
{
	Clear();
	std::string srcTypeName;
	std::string dstTypeName = "CameraManager"; 
	if (!LoadArrayFromFile(srcTypeName, pFile))
		return false;
	if (srcTypeName.compare(dstTypeName) != 0)
		return false;

	UINT64 cnt = 0;
	if (!LoadTypeFromFile(cnt, pFile))
		return false;

	for (UINT32 i = 0; i < cnt; ++i) {

		std::string camera_name;
		if (!LoadArrayFromFile(camera_name, pFile))
			return false;

		KCamera* pCamera = new KCamera();
		if (!pCamera->Load(pFile)) {
			delete pCamera;
			return false;
		}

		mCameras[camera_name] = pCamera;
	}
	return true;
}

void CameraManager::Clear()
{
	CAMERA_NAME_TO_PTR::iterator it = mCameras.begin();
	for (; it != mCameras.end(); ++it) {
		delete it->second;
	}
	mCameras.clear();
	mCameraIter = mCameras.end();
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
