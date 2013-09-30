
void RayIntersectStaticTriArray(float% ray_org[], float% ray_dir[], float% tri_pos[], float% tuv[], int cnt)
{
	int pos_idx = 0;
	int tuv_idx = 0;
	for (int tri_i = 0; tri_i < cnt; tri_i = tri_i+1) {

		pos_idx = tri_i * 9;
		tuv_idx = tri_i * 3;
		tuv[tuv_idx+0] = -3.402823466;
		bool is_valid = true;
		float edge1[3], edge2[3], tvec[3], pvec[3], qvec[3], nface[3];
		float det,inv_det;

		/* find vectors for two edges sharing vert0 */
		//SUB(edge1, vert1, vert0);
		edge1[0] = tri_pos[pos_idx+3] - tri_pos[pos_idx+0];
		edge1[1] = tri_pos[pos_idx+4] - tri_pos[pos_idx+1];
		edge1[2] = tri_pos[pos_idx+5] - tri_pos[pos_idx+2];
		//SUB(edge2, vert2, vert0);
		edge2[0] = tri_pos[pos_idx+6] - tri_pos[pos_idx+0];
		edge2[1] = tri_pos[pos_idx+7] - tri_pos[pos_idx+1];
		edge2[2] = tri_pos[pos_idx+8] - tri_pos[pos_idx+2];

		/* begin calculating determinant - also used to calculate U parameter */
		//CROSS(pvec, dir, edge2);
		pvec[0] = ray_dir[1]*edge2[2] - ray_dir[2]*edge2[1];
		pvec[1] = ray_dir[2]*edge2[0] - ray_dir[0]*edge2[2];
		pvec[2] = ray_dir[0]*edge2[1] - ray_dir[1]*edge2[0];
		//CROSS(nface, edge1, edge2);
		nface[0] = edge1[1]*edge2[2] - edge1[2]*edge2[1];
		nface[1] = edge1[2]*edge2[0] - edge1[0]*edge2[2];
		nface[2] = edge1[0]*edge2[1] - edge1[1]*edge2[0];

		float dot_nd = nface[0]*ray_dir[0] + nface[1]*ray_dir[1] + nface[2]*ray_dir[2];
		if (dot_nd > 0)
			is_valid = false; // Backward face, ignore it

		/* if determinant is near zero, ray lies in plane of triangle */
		//det = DOT(edge1, pvec);
		det = edge1[0]*pvec[0] + edge1[1]*pvec[1] + edge1[2]*pvec[2];

		if (det > -0.000001 && det < 0.000001)
			is_valid = false;
		inv_det = 1.0f / det;

		/* calculate distance from vert0 to ray origin */
		//SUB(tvec, orig, vert0);
		tvec[0] = ray_org[0] - tri_pos[pos_idx+0];
		tvec[1] = ray_org[1] - tri_pos[pos_idx+1];
		tvec[2] = ray_org[2] - tri_pos[pos_idx+2];

		/* calculate U parameter and test bounds */
		//*u = DOT(tvec, pvec) * inv_det;
		tuv[tuv_idx+1] = (tvec[0]*pvec[0] + tvec[1]*pvec[1] + tvec[2]*pvec[2]) * inv_det;
		if (tuv[tuv_idx+1] < 0.0 || tuv[tuv_idx+1] > 1.0)
			is_valid = false;

		/* prepare to test V parameter */
		//CROSS(qvec, tvec, edge1);
		qvec[0] = tvec[1]*edge1[2] - tvec[2]*edge1[1];
		qvec[1] = tvec[2]*edge1[0] - tvec[0]*edge1[2];
		qvec[2] = tvec[0]*edge1[1] - tvec[1]*edge1[0];

		/* calculate V parameter and test bounds */
		//*v = DOT(dir, qvec) * inv_det;
		tuv[tuv_idx+2] = (ray_dir[0]*qvec[0] + ray_dir[1]*qvec[1] + ray_dir[2]*qvec[2]) * inv_det;
		if (tuv[tuv_idx+2] < 0.0 || tuv[tuv_idx+1] + tuv[tuv_idx+2] > 1.0)
			is_valid = false;

		/* calculate t, ray intersects triangle */
		float tmpT = (edge2[0]*qvec[0] + edge2[1]*qvec[1] + edge2[2]*qvec[2]) * inv_det;
		if (tmpT > 0 && is_valid) tuv[tuv_idx+0] = tmpT;
	}
}