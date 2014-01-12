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
		KVec3d pos;
		KVec3d lookat;
		KVec3d up;
		double xfov;
		double focal;
		KVec2d aperture;

		MotionState() {xfov = 45.0; }
	};

	// Utility class to assist the adaptive sampling of DOF(Depth of Field) and motion blur, thread should
	// have its own instance of EvalContext, it cannot be shared between threads.
	class EvalContext {
		friend class KCamera;
	public:
		
		KVec2d inScreenPos;  // position in screen coordinates(pixel uint)
		KVec2d inAperturePos;		// offset in camera plane(in unified space(-0.5, 0.5) )
		float inMotionTime;	// time defined between 0 - 1
		
		// the value of this structure is computed from the input data
		EyeRayGen mEyeRayGen;
	};
public:
	// Set the image size so that it can generate the eye rays for the specified screen coordinates.
	void SetImageSize(UINT32 w, UINT32 h);
	
	// Configure this camera as a still camera
	void SetupStillCamera(const MotionState& param);

	// Configure this camera as a moving camera so that motion blur is considered
	void SetupMovingCamera(const MotionState& starting, const MotionState& ending);

	// Evaluate the shading for the given screen coordinates and time,
	// it will also respect the settings in input EvalContext instance.
	bool EvaluateShading(TracingInstance& tracingInstance, KColor& out_clr);

	// Get pixel position(suppose the ray is shot from the center of aperture)
	bool GetScreenPosition(const KVec3& pos, KVec2& outScrPos) const;

	static void InterpolateCameraMotion(MotionState& outMotion, const MotionState& ms0, const MotionState& ms1, double cur_t);

protected:
	void ConfigEyeRayGen(EyeRayGen& outEyeRayGen, MotionState& outMotion, double cur_t) const;

private:
	MotionState mStartingState;
	MotionState mEndingState;
	bool mIsMoving;

	UINT32 mImageWidth;
	UINT32 mImageHeight;

};