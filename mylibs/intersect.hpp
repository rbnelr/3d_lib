#pragma once

#include "vector.hpp"
#include "float_precision.hpp"

namespace intersect {
	using namespace vector;
	using namespace float_precision;

	bool raycast_plane (v3 ray_pos, v3 ray_dir, v3 plane_pos, v3 plane_a, v3 plane_b, v3* hit_pos, v2* hit_uv, flt* hit_t) { // plane_normal is cross(plane_a, plane_b), hit is plane_pos + hit_uv.x * plane_a + hit_uv.y * plane_b

		v3 plane_normal = cross(plane_a, plane_b);

		plane_normal = normalize(plane_normal);

		m3 to_plane = m3::rows(	plane_a / length_sqr(plane_a),
							   plane_b / length_sqr(plane_b),
							   plane_normal);

		v3 ray_pos_rel = to_plane * (ray_pos -plane_pos);
		v3 ray_dir_rel = to_plane * ray_dir;

		if (ray_pos_rel.z == 0) {
			// ray on plane
			if (hit_uv) *hit_uv = ray_pos_rel.xy();
			if (hit_t) *hit_t = 0;
			return true;
		}

		if (ray_dir_rel.z == 0) {
			// ray parallel to plane
			return false;
		}

		if (ray_dir_rel.z * ray_pos_rel.z > 0) {
			// same sign: ray points away from plane
			return false;
		}

		flt t = ray_pos_rel.z / -ray_dir_rel.z;

		if (hit_pos) *hit_pos = ray_pos + ray_dir * t;
		if (hit_uv) *hit_uv = ray_pos_rel.xy() + t * ray_dir_rel.xy();
		if (hit_t) *hit_t = t;

		return true;
	}
	bool raycast_quad (v3 ray_pos, v3 ray_dir, v3 quad_pos, v3 quad_a, v3 quad_b, v3* hit_pos=0, v2* hit_uv=0, flt* hit_t=0) { // quad_normal is cross(quad_a, quad_b), technically the quad can also be a parallelogram
		v2 uv;
		if (!raycast_plane(ray_pos, ray_dir, quad_pos, quad_a, quad_b, hit_pos, &uv, hit_t))
			return false;

		if (hit_uv) *hit_uv = uv;

		return all(uv >= 0 && uv <= 1);
	}
	bool raycast_triangle (v3 ray_pos, v3 ray_dir, v3 tri_a, v3 tri_ab, v3 tri_ac, v3* hit_pos=0, v2* hit_uv=0, flt* hit_t=0) { // quad_normal is cross(quad_a, quad_b), technically the quad can also be a parallelogram
		v2 uv;
		if (!raycast_plane(ray_pos, ray_dir, tri_a, tri_ab, tri_ac, hit_pos, &uv, hit_t))
			return false;

		if (hit_uv) *hit_uv = uv;

		return all(uv >= 0 && uv <= 1) && (uv.x + uv.y) <= 1;
	}

	bool intersect_AABB (v3 ray_pos, v3 ray_dir_inv, v3 aabb_min, v3 aabb_max, flt* out_tmin, flt* out_tmax) {
		flt	tmin, tmax;
		{
			flt	tx1 = (aabb_min.x - ray_pos.x) * ray_dir_inv.x;
			flt	tx2 = (aabb_max.x - ray_pos.x) * ray_dir_inv.x;

			tmin = min(tx1, tx2);
			tmax = max(tx1, tx2);
		}
		{
			flt	ty1 = (aabb_min.y - ray_pos.y) * ray_dir_inv.y;
			flt	ty2 = (aabb_max.y - ray_pos.y) * ray_dir_inv.y;

			tmin = max(tmin, min(ty1, ty2));
			tmax = min(tmax, max(ty1, ty2));
		}
		{
			flt	tz1 = (aabb_min.z - ray_pos.z) * ray_dir_inv.z;
			flt	tz2 = (aabb_max.z - ray_pos.z) * ray_dir_inv.z;

			tmin = max(tmin, min(tz1, tz2));
			tmax = min(tmax, max(tz1, tz2));
		}

		*out_tmin = tmin;
		*out_tmax = tmax;
		return tmax > 0 && tmin <= tmax;
	}
	bool intersect_AABB (v3 ray_pos, v3 ray_dir_inv, v3 aabb_min, v3 aabb_max) {
		flt	tmin, tmax;
		return intersect_AABB(ray_pos, ray_dir_inv, aabb_min, aabb_max, &tmin, &tmax);
	}

	template <typename GET_VOXEL>
	bool raycast_voxels (GET_VOXEL get_voxel, v3 ray_pos, v3 ray_dir, iv3 pos_min, iv3 pos_max) { // GET_VOXEL: bool 
		// iv3* out_hit_voxel, v3* hit_pos, iv3* out_face_normal
		
		iv3 step_delta = iv3(	(int)normalize(ray_dir.x),
								(int)normalize(ray_dir.y),
								(int)normalize(ray_dir.z) );
		
		v3 step = v3(			length(ray_dir / abs(ray_dir.x)),
								length(ray_dir / abs(ray_dir.y)),
								length(ray_dir / abs(ray_dir.z)) );
		step = select(ray_dir != 0, step, INF); // fix nan

		iv3 cur_block		= (iv3)floor(ray_pos);
		v3	pos_in_block	= ray_pos -floor(ray_pos);

		v3 next = step * select(ray_dir > 0, 1 -pos_in_block, pos_in_block);

		auto find_next_axis = [&] (v3 next) {
			if (		next.x < next.y && next.x < next.z )	return 0;
			else if (	next.y < next.z )						return 1;
			else												return 2;
		};

		int first_axis = find_next_axis(next);

		for (;;) {
			
			if (any(cur_block < pos_min || cur_block >= pos_max))
				break;

			if (!get_voxel(cur_block, pos_in_block, 0)) {
				//*hit_block = cur_block;
				//*hit_face = face;
				//*out_hit_voxel = cur_block;
				return true;
			}
		
			int axis = find_next_axis(next); // find which axis plane we intersect next
			
			//face = (block_face_e)(axis*2 +(step_delta[axis] < 0 ? 1 : 0));
			
			//if (next[axis] > max_ray_dist) return false;
			
			next[axis] += step[axis]; // step along ray for next axis
			cur_block[axis] += step_delta[axis]; // step voxel coord
		}
		
		#if 0
		v3 ray_pos_floor = floor(ray_pos);

		v3 pos_in_block = ray_pos -ray_pos_floor;

		v3 next = step * select(ray_dir > 0, 1 -pos_in_block, pos_in_block);
		next = select(ray_dir != 0, next, INF);

		auto find_next_axis = [&] (v3 next) {
			if (		next.x < next.y && next.x < next.z )	return 0;
			else if (	next.y < next.z )						return 1;
			else												return 2;
		};

		iv3 cur_block = (iv3)ray_pos_floor;

		int first_axis = find_next_axis(next);
		block_face_e face = (block_face_e)(first_axis*2 +(step_delta[first_axis] > 0 ? 1 : 0));

		for (;;) {

			//highlight_block(cur_block);
			Block* b = query_block(cur_block);
			if (block_props[b->type].breakable) {
				*hit_block = cur_block;
				*hit_face = face;
				return b;
			}

			int axis = find_next_axis(next);

			face = (block_face_e)(axis*2 +(step_delta[axis] < 0 ? 1 : 0));

			if (next[axis] > max_dist) return nullptr;

			next[axis] += step[axis];
			cur_block[axis] += step_delta[axis];
		}
		#endif

		return false;
	}
}
