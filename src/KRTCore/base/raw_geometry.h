/************************************************************************/
//  [4/14/2011 Kai]
// The class and functions in this header is in order to handle the mesh conversion.  
// I assume the input geometry is the plain and unoptimized format, it will be converted into
// the optimized internal format for ray tracing.
/************************************************************************/
#pragma once
#include "geometry.h"

namespace Geom {
	struct FaceVertIndex {
		UINT32 idx[3];
	};



	struct KRT_API RawMesh {
		std::vector<FaceVertIndex> mFacePosIdx;
		std::vector<KVec3> mPosData;

		std::vector<FaceVertIndex> mFaceNorIdx;
		std::vector<KVec3> mNorData;

		std::vector<FaceVertIndex> mFaceTexIdx;
		std::vector<KVec2> mTexData;

		void SetFaceCnt(UINT32 cnt, bool needTex);
		void ComputeFaceTangent(UINT32 faceIdx, KVec3& tangent, KVec3& binormal) const;
		void ComputeFaceNormal(UINT32 idx, KVec3& out_nor) const;

		bool Detach(const std::vector<UINT32>& faces, RawMesh& outMesh) const;
		bool BuildNormals(const std::vector<UINT32> smGroups);
		
	};

	void KRT_API CompileOptimizedMesh(const RawMesh& raw_mesh, KTriMesh& out_mesh);
}