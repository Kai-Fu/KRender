#include "raw_geometry.h"
#include "../util/memory_pool.h"
#include <assert.h>

namespace Geom {

	void RawMesh::SetFaceCnt(UINT32 cnt, bool needTex) {
		mFacePosIdx.resize(cnt); 
		mFaceNorIdx.resize(cnt); 
		if (needTex)
			mFaceTexIdx.resize(cnt);
	}

	bool RawMesh::Detach(const std::vector<UINT32>& faces, RawMesh& outMesh) const {
		std::vector<UINT32> vertexLUT(mPosData.size());
		for (size_t i = 0; i < vertexLUT.size(); ++i) 
			vertexLUT[i] = INVALID_INDEX;

		std::vector<UINT32> normalLUT(mNorData.size());
		for (size_t i = 0; i < normalLUT.size(); ++i) 
			normalLUT[i] = INVALID_INDEX;

		std::vector<UINT32> texcrdLUT(mTexData.size());
		for (size_t i = 0; i < texcrdLUT.size(); ++i) 
			texcrdLUT[i] = INVALID_INDEX;

		RawMesh* pRet = &outMesh;
		pRet->SetFaceCnt((UINT32)faces.size(), !mFaceTexIdx.empty());

		UINT32 vertCnt = 0;
		UINT32 normCnt = 0;
		UINT32 texcrdCnt = 0;
		for (size_t fi = 0; fi < faces.size(); ++fi) {

			if (faces[fi] >= mFacePosIdx.size()) 
				return false;

			for (int i = 0; i < 3; ++i) {
				UINT32& vertIdx = vertexLUT[mFacePosIdx[faces[fi]].idx[i]];
				if (vertIdx == INVALID_INDEX)
					vertIdx = vertCnt++;
				pRet->mFacePosIdx[fi].idx[i] = vertIdx;

				UINT32& normIdx = normalLUT[mFaceNorIdx[faces[fi]].idx[i]];
				if (normIdx == INVALID_INDEX)
					normIdx = normCnt++;
				pRet->mFaceNorIdx[fi].idx[i] = normIdx;

				if (!mFaceTexIdx.empty() && mFaceTexIdx[faces[fi]].idx[i] != INVALID_INDEX) {
					UINT32& texIdx = texcrdLUT[mFaceTexIdx[faces[fi]].idx[i]];
					if (texIdx == INVALID_INDEX)
						texIdx = texcrdCnt++;
					pRet->mFaceTexIdx[fi].idx[i] = texIdx;
				}

			}
		}

		pRet->mPosData.resize(vertCnt);
		for (size_t i = 0; i < vertexLUT.size(); ++i) {
			if (vertexLUT[i] != INVALID_INDEX)
				pRet->mPosData[vertexLUT[i]] = mPosData[i];
		}

		pRet->mNorData.resize(normCnt);
		for (size_t i = 0; i < normalLUT.size(); ++i) {
			if (normalLUT[i] != INVALID_INDEX)
				pRet->mNorData[normalLUT[i]] = mNorData[i];
		}

		if (texcrdCnt == 0) {
			pRet->mFaceTexIdx.clear();
			pRet->mTexData.clear();
		}

		if (!pRet->mFaceTexIdx.empty()) {
			pRet->mTexData.resize(texcrdCnt);
			for (size_t i = 0; i < texcrdLUT.size(); ++i) {
				if (texcrdLUT[i] != INVALID_INDEX)
					pRet->mTexData[texcrdLUT[i]] = mTexData[i];
			}
		}

		return true;
	}


	struct tSMGNormal
	{
		UINT32 smGroup;
		std::vector<KVec3> normal;
		UINT32 buf_idx;

		tSMGNormal() {normal.reserve(4);}
	};

	class VertexNormalSet : public std::vector<tSMGNormal>
	{
	public:
		VertexNormalSet() {reserve(3);}
	};

	bool CompareKVec3(const KVec3& ref0, const KVec3& ref1) {

		for (int i = 0; i < 3; ++i)
			if (ref0[i] != ref1[i])
				return ref0[i] < ref1[i];
		return false;

	}

	void RawMesh::ComputeFaceNormal(UINT32 idx, KVec3& out_nor) const
	{
		const KVec3& v0 = mPosData[mFacePosIdx[idx].idx[0]];
		const KVec3& v1 = mPosData[mFacePosIdx[idx].idx[1]];
		const KVec3& v2 = mPosData[mFacePosIdx[idx].idx[2]];

		KVec3 edge0 = v1 - v0;
		KVec3 edge1 = v2 - v1;
		out_nor = edge0 ^ edge1;
		out_nor.normalize();
	}

	bool RawMesh::BuildNormals(const std::vector<UINT32> smGroups)
	{
		// Compute the normals regarding the input smooth group
		if (smGroups.size() != mFacePosIdx.size())
			return false;

		// Need to clear the normal
		mNorData.clear();

		// Add each face's normal & area into vertex normal set
		std::vector<VertexNormalSet> vert_normals(mPosData.size());
		for (UINT32 fi = 0; fi < mFacePosIdx.size(); ++fi) {
			KVec3 face_nor;
			ComputeFaceNormal(fi, face_nor);

			for (UINT32 vi = 0; vi < 3; ++vi) {
				UINT32 vidx = mFacePosIdx[fi].idx[vi];
				bool found =false;
				for (VertexNormalSet::iterator it = vert_normals[vidx].begin(); it != vert_normals[vidx].end(); ++it) {
					if (it->smGroup & smGroups[fi]) {
						it->normal.push_back(face_nor);
						it->smGroup |= smGroups[fi];
						found = true;
						break;
					}
				}
				if (!found && smGroups[fi] != 0) {
					tSMGNormal nor;
					nor.smGroup = smGroups[fi];
					nor.normal.push_back(face_nor);
					vert_normals[vidx].push_back(nor);
				}
			}
		}

		// Compute the buffer index for each normal that belongs to different smooth group
		UINT32 base_index = 0;
		for (UINT32 vi = 0; vi < mFacePosIdx.size(); ++vi) {
			for (VertexNormalSet::iterator it = vert_normals[vi].begin(); it != vert_normals[vi].end(); ++it) {
				std::sort(it->normal.begin(), it->normal.end(), CompareKVec3);

				KVec3& last_nor = it->normal[0];
				std::vector<KVec3>::iterator it_nor = it->normal.begin();
				KVec3 smooth_normal(*it_nor);
				++it_nor;
				for (; it_nor != it->normal.end(); ++it_nor) {
					if (nvmath::lengthSquared(*it_nor - last_nor) < 0.001f)
						continue;
					last_nor = *it_nor;
					smooth_normal += *it_nor;
				}
				smooth_normal.normalize();
				it->normal[0] = smooth_normal;
				it->buf_idx = base_index++;
			}
		}
		mNorData.resize(base_index);
		for (UINT32 vi = 0; vi < mPosData.size(); ++vi) {
			for (VertexNormalSet::iterator it = vert_normals[vi].begin(); it != vert_normals[vi].end(); ++it) {
				if (!it->normal.empty()) {
					mNorData[it->buf_idx] = it->normal[0];
					mNorData[it->buf_idx].normalize();
				}
			}
		}

		// Now assign each face's normal index
		for (UINT32 fi = 0; fi < mFacePosIdx.size(); ++fi) {

			if (smGroups[fi] == 0) {
				// Use the face normal for smooth group 0
				KVec3 face_nor;
				ComputeFaceNormal(fi, face_nor);
				UINT32 nor_idx = (UINT32)mNorData.size();
				mNorData.push_back(face_nor);
				mFaceNorIdx[fi].idx[0] = nor_idx;
				mFaceNorIdx[fi].idx[1] = nor_idx;
				mFaceNorIdx[fi].idx[2] = nor_idx;
				continue;
			}

			UINT32 vi = 0;
			for (; vi < 3; ++vi) {
				UINT32 vidx = mFacePosIdx[fi].idx[vi];

				for (VertexNormalSet::iterator it = vert_normals[vidx].begin(); it != vert_normals[vidx].end(); ++it) {
					if (it->smGroup & smGroups[fi]) {
						mFaceNorIdx[fi].idx[vi] = it->buf_idx;
						break;
					}
				}

			}

			assert(vi == 3);

		}

		return true;
	}


	void RawMesh::ComputeFaceTangent(UINT32 faceIdx, KVec3& tangent, KVec3& binormal) const
	{
		const KVec3 v1 = mPosData[mFacePosIdx[faceIdx].idx[0]];
		const KVec3 v2 = mPosData[mFacePosIdx[faceIdx].idx[1]];
		const KVec3 v3 = mPosData[mFacePosIdx[faceIdx].idx[2]];

		const KVec2 w1 = mTexData[mFaceTexIdx[faceIdx].idx[0]];
		const KVec2 w2 = mTexData[mFaceTexIdx[faceIdx].idx[1]];
		const KVec2 w3 = mTexData[mFaceTexIdx[faceIdx].idx[2]];

		float x1 = v2[0] - v1[0];
		float x2 = v3[0] - v1[0];
		float y1 = v2[1] - v1[1];
		float y2 = v3[1] - v1[1];
		float z1 = v2[2] - v1[2];
		float z2 = v3[2] - v1[2];

		float s1 = w2[0] - w1[0];
		float s2 = w3[0] - w1[0];
		float t1 = w2[1] - w1[1];
		float t2 = w3[1] - w1[1];

		float d = s1 * t2 - s2 * t1;
		if (fabs(d) < 0.000001f) 
			d = 1.0f;

		float r = 1.0f / d;
		tangent = KVec3(
			(t2 * x1 - t1 * x2) * r, 
			(t2 * y1 - t1 * y2) * r,
			(t2 * z1 - t1 * z2) * r);

		binormal = KVec3(
			(s1 * x2 - s2 * x1) * r, 
			(s1 * y2 - s2 * y1) * r,
			(s1 * z2 - s2 * z1) * r);

		tangent.normalize();
		binormal.normalize();
	}

	struct SortableVec3 {
		KVec3 vec;
		UINT32 idx;

		static bool CompareVec3(const KVec3& left, const KVec3& right) {
			if (left[0] != right[0])
				return left[0] < right[0];
			else if (left[1] != right[1])
				return left[1] < right[1];
			else if (left[2] != right[2])
				return left[2] < right[2];
			else 
				return false;
		}

		static bool  isNormalEqual(const KVec3& left, const KVec3& right) {
			float dotSqr = left * right;
			dotSqr *= dotSqr;
			float dotVec = (left * left) * (right * right);
			if (dotSqr > (dotVec * 0.99f))
				return true;
			else
				return false;
		}

		bool operator < (const SortableVec3& ref) const {
			return CompareVec3(vec, ref.vec);
		}

	};

	template <typename T>
	struct DataList {
		T data;
		DataList* next;
	};

	struct SortablePN {
		KVec3 pos;
		DataList<SortableVec3> nor_list;
	};

	void _compile_pos_nor_data(const RawMesh& raw_mesh, KTriMesh& out_mesh)
	{
		GlowableMemPool mem_pool;
		// PN index buffer generation
		std::vector<SortablePN> pn_data;
		out_mesh.mFaces.resize(raw_mesh.mFacePosIdx.size());
		pn_data.resize(raw_mesh.mPosData.size());
		for (size_t i = 0; i < pn_data.size(); ++i) {
			pn_data[i].nor_list.data.idx = INVALID_INDEX;
			pn_data[i].nor_list.next = NULL;
			pn_data[i].pos = raw_mesh.mPosData[i];

		}

		UINT32 pn_index = 0;
		for (size_t i = 0; i < raw_mesh.mFacePosIdx.size(); ++i) {

			for (int v = 0; v < 3; ++v) {
				int p_idx = raw_mesh.mFacePosIdx[i].idx[v];
				int n_idx = raw_mesh.mFaceNorIdx[i].idx[v];
				const KVec3& nor = raw_mesh.mNorData[n_idx];
				SortablePN& pos_it = pn_data[p_idx];
				if (pos_it.nor_list.data.idx == INVALID_INDEX) {
					pos_it.nor_list.data.idx = pn_index;
					pos_it.nor_list.data.vec = nor;
					out_mesh.mFaces[i].pn_idx[v] = pn_index;
					++pn_index;
				}
				else {
					DataList<SortableVec3>* nor_it = &pos_it.nor_list;
					DataList<SortableVec3>** next_ptr;
					while (nor_it != NULL) {
						if (SortableVec3::isNormalEqual(nor_it->data.vec, nor)) {
							out_mesh.mFaces[i].pn_idx[v] = nor_it->data.idx;
							break;
						}
						next_ptr = &nor_it->next;
						nor_it = nor_it->next;
					}

					if (nor_it == NULL) {
						SortableVec3 t_nor = {nor, pn_index};
						out_mesh.mFaces[i].pn_idx[v] = pn_index;
						*next_ptr =(DataList<SortableVec3>*)mem_pool.Alloc(sizeof(DataList<SortableVec3>));
						(*next_ptr)->data = t_nor;
						(*next_ptr)->next = NULL;
						++pn_index;
					}
					
				}
			}
		}

		// PN data array
		out_mesh.mVertPN.resize(pn_index);
		for (size_t i = 0; i < pn_data.size(); ++i) {

			const SortablePN& pos_it = pn_data[i];
			if (pos_it.nor_list.data.idx == INVALID_INDEX)
				continue;

			const DataList<SortableVec3>* nor_it = &pos_it.nor_list;
			while (nor_it != NULL) {
				KTriMesh::PN_Data pn = {pos_it.pos, nor_it->data.vec};
				//assert(nor_it->data.idx < pn_index);
				out_mesh.mVertPN[nor_it->data.idx] = pn;
				nor_it = nor_it->next;
				--pn_index;
			}
			
		}

	}

	struct Nor_TT {
		KVec3 nor;
		UINT32 share_cnt;
		KVec3 tangent;
		UINT32 buf_idx;
		KVec3 binormal;

		bool TryAdd(const KVec3& v_n, const KVec3& face_t, const KVec3& face_b) {
			if (SortableVec3::isNormalEqual(nor, v_n)) {
				if (face_t * tangent < 0.50f || face_b * binormal < 0.50f)
					return false;

				float t = 1.0f / float(share_cnt + 1);
				tangent *= (1.0f - t);
				tangent += face_t * t;
				binormal *= (1.0f - t);
				binormal += face_b * t;
				++share_cnt;

				return true;
			}
			else
				return false;
		}

	
	};

	struct SortableTT {
		KVec2 uv;
		DataList<Nor_TT> tt_list;
	};

	void _compile_tex_tangent_data(const RawMesh& raw_mesh, KTriMesh& out_mesh) {

		GlowableMemPool mem_pool;
		if (raw_mesh.mFaceTexIdx.empty())
			return;

		std::vector<SortableTT> tt_data;
		tt_data.resize(raw_mesh.mTexData.size());
		for (size_t i = 0; i < tt_data.size(); ++i)  {
			tt_data[i].tt_list.data.buf_idx = INVALID_INDEX;
			tt_data[i].tt_list.next = NULL;
			tt_data[i].uv = raw_mesh.mTexData[i];
		}

		out_mesh.mTexFaces.resize(raw_mesh.mFaceTexIdx.size());
		UINT32 tt_index = 0;
		for (size_t i = 0; i < raw_mesh.mFaceTexIdx.size(); ++i) {

			for (int v = 0; v < 3; ++v) {
				UINT32 t_idx = raw_mesh.mFaceTexIdx[i].idx[v];
				UINT32 n_idx = raw_mesh.mFaceNorIdx[i].idx[v];
				KVec2 uv = raw_mesh.mTexData[t_idx];
				KVec3 nor = raw_mesh.mNorData[n_idx];
				KVec3 tangent, binormal;
				raw_mesh.ComputeFaceTangent((UINT32)i, tangent, binormal);
				SortableTT& tt = tt_data[t_idx];
				Nor_TT temp_ntt = {nor, 1, tangent, tt_index, binormal};
				if (tt.tt_list.data.buf_idx == INVALID_INDEX) {
					tt.tt_list.data = temp_ntt;
					out_mesh.mTexFaces[i].tt_idx[v] = tt_index;
					++tt_index;
				}
				else {
					DataList<Nor_TT>* it_ntt = &tt.tt_list;
					DataList<Nor_TT>** next_ptr;
					while (it_ntt != NULL) {
						if (it_ntt->data.TryAdd(nor, tangent, binormal)) {
							out_mesh.mTexFaces[i].tt_idx[v] = it_ntt->data.buf_idx;
							break;
						}
						next_ptr = &it_ntt->next;
						it_ntt = it_ntt->next;
					}
					if (it_ntt == NULL) {
						
						*next_ptr = (DataList<Nor_TT>*)mem_pool.Alloc(sizeof(DataList<Nor_TT>));
						(*next_ptr)->data = temp_ntt;
						(*next_ptr)->next = NULL;
						out_mesh.mTexFaces[i].tt_idx[v] = tt_index;
						++tt_index;
					}


				}
			}
		}

		// Fill the uv & tangent data array
		out_mesh.mVertTT.resize(tt_index);
		for (size_t i = 0; i < tt_data.size(); ++i) {

			const SortableTT* tt_it = &tt_data[i];
			if (tt_it->tt_list.data.buf_idx == INVALID_INDEX)
				continue;

			const DataList<Nor_TT>* nt_it = &tt_it->tt_list;
			while (nt_it != NULL) {
				KTriMesh::TT_Data tt = {tt_it->uv, nt_it->data.tangent, nt_it->data.binormal};
				assert(nt_it->data.buf_idx < tt_index);
				out_mesh.mVertTT[nt_it->data.buf_idx] = tt;
				nt_it = nt_it->next;
			}
		}
	}

	void CompileOptimizedMesh(const RawMesh& raw_mesh, KTriMesh& out_mesh)
	{
		_compile_pos_nor_data(raw_mesh, out_mesh);
		_compile_tex_tangent_data(raw_mesh, out_mesh);
	}

}