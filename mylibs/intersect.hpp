#pragma once

#include "vector.hpp"
#include "float_precision.hpp"

namespace intersect {
	using namespace vector;
	using namespace float_precision;

	bool intersect (v2 point, v2 rect_a, v2 rect_b, v2* hit_uv=0) {
		v2 uv = map(point, rect_a, rect_b);
		*hit_uv = uv;
		return all(uv >= 0 && uv <= 1);
	}

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

			tmin = MIN(tx1, tx2);
			tmax = MAX(tx1, tx2);
		}
		{
			flt	ty1 = (aabb_min.y - ray_pos.y) * ray_dir_inv.y;
			flt	ty2 = (aabb_max.y - ray_pos.y) * ray_dir_inv.y;

			tmin = MAX(tmin, MIN(ty1, ty2));
			tmax = MIN(tmax, MAX(ty1, ty2));
		}
		{
			flt	tz1 = (aabb_min.z - ray_pos.z) * ray_dir_inv.z;
			flt	tz2 = (aabb_max.z - ray_pos.z) * ray_dir_inv.z;

			tmin = MAX(tmin, MIN(tz1, tz2));
			tmax = MIN(tmax, MAX(tz1, tz2));
		}

		*out_tmin = tmin;
		*out_tmax = tmax;
		return tmax > 0 && tmin <= tmax;
	}
	bool intersect_AABB (v3 ray_pos, v3 ray_dir_inv, v3 aabb_min, v3 aabb_max) {
		flt	tmin, tmax;
		return intersect_AABB(ray_pos, ray_dir_inv, aabb_min, aabb_max, &tmin, &tmax);
	}

	constexpr iv3 face_normals[6] = {
		iv3(+1,0,0),
		iv3(-1,0,0),
		iv3(0,+1,0),
		iv3(0,-1,0),
		iv3(0,0,+1),
		iv3(0,0,-1),
	};
	template <typename GET_VOXEL>
	bool raycast_voxels (GET_VOXEL raycast_voxel, v3 ray_pos, v3 ray_dir, flt max_ray_dist=INF, iv3* out_hit_voxel=0, v3* out_hit_pos=0, int* out_hit_face=0) { // if ray starts inside block then the next block will be the first
		
		ray_dir = normalize(ray_dir);

		iv3 step_delta = iv3(	(int)normalize(ray_dir.x),
								(int)normalize(ray_dir.y),
								(int)normalize(ray_dir.z) ); // step direction in our voxel grid (we step one axis at a time) 
		
		v3 step = v3(			length(ray_dir / abs(ray_dir.x)),
								length(ray_dir / abs(ray_dir.y)),
								length(ray_dir / abs(ray_dir.z)) ); // how far along the ray we need to step to move by 1 voxel for each axis
		step = select(ray_dir != 0, step, INF); // fix nan

		iv3 cur_block = (iv3)floor(ray_pos); // voxel we consider the ray to start in

		v3 pos_in_block	= ray_pos -(v3)cur_block;

		v3 next = step * select(ray_dir > 0, 1 -pos_in_block, pos_in_block); // next holds the distances along the ray of the next axis plane intersection (ie. voxel change) for each axis

		auto find_next_axis = [&] (v3 next) { // the smallest element in next is the axis we change voxels along (ie. the closest axis plane intersection alogn the ray)
			return smallest_comp(next);
		};

		int prev_axis = biggest_comp(next -step); // axis through which the cur_block was entered ( we want the biggest element in the vector, since we need the latest intersection)

		for (;;) {
			
			int axis = find_next_axis(next); // find which axis plane we intersect next

			v3 hit_pos = pos_in_block;
			int hit_face = prev_axis * 2 +(ray_dir[prev_axis] < 0 ? 0 : 1); // face index corresponding to face_normals array

			if (raycast_voxel(cur_block, hit_pos, hit_face)) { // call user handler for each voxel, user returns if this should be the final voxel hit
				if (out_hit_voxel)	*out_hit_voxel = cur_block; // out params hold info of last voxel hit
				if (out_hit_pos)	*out_hit_pos = hit_pos;
				if (out_hit_face)	*out_hit_face = hit_face;
				return true;
			}
		
			if (next[axis] > max_ray_dist) // stop if the next voxel cannot be hit within a distace of max_ray_dist
				return false;

			cur_block[axis] += step_delta[axis]; // step through voxels

			pos_in_block = (ray_pos + next[axis] * ray_dir) -(v3)cur_block; // calculate the position of the next hit relative to the next voxel
			prev_axis = axis;

			next[axis] += step[axis]; // step along ray
		}
	}
}
