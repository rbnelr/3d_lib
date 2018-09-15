#pragma once

#include "3d_lib/engine_include.hpp"
#include "dear_imgui.hpp"

namespace engine {

struct Camera2D {
	v2		pos_world = 0;
	flt		rot = 0;

	flt		base_vsize_world = 1;

	flt		clip_near = -1;
	flt		clip_far = 100;

	v2		_size_world;

	v2 get_size_world () { return _size_world; }
	// movement
	bool	fixed = false; // camera not controllable

	flt		zoom_log = 0;
	
	flt		base_speed = 0.2f;
	flt		fast_mult = 4;

	flt		rot_vel = deg(45);
	flt		zoom_log_speed = 0.1f;

	bool	dragging = false;
	v2		grab_pos_world;

	void zoom_to_vsize_world (flt vsize_world) {
		flt zoom_factor = base_vsize_world / vsize_world;
		zoom_log = log2(zoom_factor);
	}

	void update (Input& inp, flt dt) {
		update(inp, dt, inp.get_window_screen_rect());
	}

	void update (Input& inp, flt dt, Screen_Rect const& rect) {
		
		if (imgui::TreeNodeEx("Camera2D", ImGuiTreeNodeFlags_CollapsingHeader|(0 ? ImGuiTreeNodeFlags_DefaultOpen : 0))) {
		
			imgui::DragFloat2("pos_world", &pos_world.x, 1.0f / 100);
			imgui::DragFloat("rot", &rot, 1.0f / 100);
		
			imgui::DragFloat("zoom_log", &zoom_log, 1.0f / 100);
		
			imgui::DragFloat("clip_near", &clip_near, 1.0f / 100);
			imgui::DragFloat("clip_far", &clip_far, 1.0f / 100);
		
			imgui::DragFloat("base_speed", &base_speed, 1.0f / 10);
			imgui::DragFloat("fast_mult", &fast_mult, 1.0f / 10);
		}
		
		if (inp.went_down('R')) { // Reset camera
			pos_world = 0;
			rot = 0;
			zoom_log = 0;
		}

		if (!fixed) {
			flt roll_vel = rot_vel; // ang / sec

			int roll_dir = 0;
			if (inp.is_down('Q')) roll_dir -= 1;
			if (inp.is_down('E')) roll_dir += 1;

			rot += (flt)-roll_dir * roll_vel * dt;
			rot = mymod(rot, deg(360));

			zoom_log += inp.get_mousewheel_delta() * zoom_log_speed;
		}
			
		flt zoom_factor = powf(2, zoom_log);
		flt vradius = (base_vsize_world * 0.5f) / zoom_factor;
			
		if (!fixed) {
			v2 mouse_clip = inp.mouse_cursor_pos_clip(rect);

			if (inp.is_down(GLFW_MOUSE_BUTTON_LEFT)) {
				v2 mouse_pos_cam = mouse_clip * vradius * v2((flt)rect.size_px.x / (flt)rect.size_px.y, 1);

				if (!dragging) {
					grab_pos_world = (rotate2(rot) * mouse_pos_cam) +pos_world;
					dragging = true;
				}

				if (dragging) {
					pos_world = grab_pos_world -(rotate2(rot) * mouse_pos_cam);
				}
			} else if (dragging) {

				dragging = false;
			}

			// movement
			iv2 move_dir = 0;
			if (inp.is_down('S'))	move_dir.y -= 1;
			if (inp.is_down('W'))	move_dir.y += 1;
			if (inp.is_down('A'))	move_dir.x -= 1;
			if (inp.is_down('D'))	move_dir.x += 1;

			hm cam_to_world = calc_cam_to_world();

			flt speed = base_speed * (inp.is_down(GLFW_KEY_LEFT_SHIFT) ? fast_mult : 1);

			pos_world += cam_to_world.m2() * (normalize_or_zero((v2)move_dir) * speed) * dt;
		}

		calc_matricies(vradius, rect);
	}

	hm calc_cam_to_world () {
		return translateH(v3(pos_world,0)) * hm(rotate2(rot));
	}
	hm calc_world_to_cam () {
		return hm(rotate2(-rot)) * translateH(-v3(pos_world,0));
	}

	hm		world_to_cam;
	m4		cam_to_clip;

	void calc_matricies (flt vradius, Screen_Rect const& rect) {
		world_to_cam = calc_world_to_cam();

		v2 radius = vradius * v2((flt)rect.size_px.x / (flt)rect.size_px.y, 1);

		_size_world = radius * 2;

		cam_to_clip = engine::calc_orthographic_matrix(radius, clip_near, clip_far);
	}
};

}
