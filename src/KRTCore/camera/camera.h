/************************************************************************/
// Created by Kai on 24/Mar/2011
// I hate the old implementation for camera handling, so I'm going to re-factory it.
/************************************************************************/
#pragma once
#include "../base/BaseHeader.h"
#include "../base/geometry.h"
#include "../intersection/EyeRayGen.h"

#include "../image/color.h"

class TracingInstance;

/************************************************************************/
// Instance of KCamera is the main entry for all the interfaces related to 
// camera handling, in addition to the basic camera properties, this class 
// also handles DOF and motion blur. It provides the EvalContext to assist
// the adaptive sampling algorithm of DOF running on thread's local storage.
/************************************************************************/
class KCamera
{
public:
	KCamera();
	virtual ~KCamera();
	

	struct MotionState {
		KVec3 pos;
		KVec3 lookat;
		KVec3 up;
		float xfov;
		float focal;

		MotionState() {xfov = 45.0f; }
	};

	// Utility class to assist the adaptive sampling of DOF(Depth of Field) and motion blur, thread should
	// have its own instance of EvalContext, it cannot be shared between threads.
	class EvalContext {
		friend class KCamera;
	public:
		
		KVec2 inScreenPos;  // position in screen coordinates(pixel uint)
		KVec2 inAperturePos;		// offset in camera plane(world unit)
		float inMotionTime;	// time defined between 0 - 1
		
		// the value of this structure is computed from the input data
		EyeRayGen mEyeRayGen;
	};
public:
	// Set the image size so that it can generate the eye rays for the specified screen coordinates.
	void SetImageSize(UINT32 w, UINT32 h);

	// Get the aperture size of the camera, default is pinhole camera(-1, -1)
	const KVec2& GetApertureSize() const;
	void SetApertureSize(float x, float y);
	
	// Configure this camera as a still camera
	void SetupStillCamera(const MotionState& param);

	// Configure this camera as a moving camera so that motion blur is considered
	void SetupMovingCamera();
	void AddCameraMotionState(const MotionState& param);

	// Evaluate the shading for the given screen coordinates and time,
	// it will also respect the settings in input EvalContext instance.
	bool EvaluateShading(TracingInstance& tracingInstance, KColor& out_clr);

	// Shot one ray with the given aperture position
	void CastRay(TracingInstance& tracingInstance, float x, float y, const KVec2& aperturePos, KRay& outRay) const;

	// Get pixel position(suppose the ray is shot from the center of aperture)
	bool GetScreenPosition(const KVec3& pos, KVec2& outScrPos) const;
	// File IO functions
	bool Save(FILE* pFile);
	bool Load(FILE* pFile);

protected:
	void InterpolateCameraMotion(MotionState& outMotion, float t) const;
	void ConfigEyeRayGen(EyeRayGen& outEyeRayGen, MotionState& outMotion, float t) const;
private:
	std::vector<MotionState> mMotionStates;
	KVec2 mApertureSize; // aperture size may be different in x and y directions

	UINT32 mImageWidth;
	UINT32 mImageHeight;

};