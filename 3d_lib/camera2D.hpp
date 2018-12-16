#pragma once

#include "3d_lib/engine_include.hpp"
#include "dear_imgui.hpp"

namespace engine {

class Camera2D {
public:
	//// 
	v2		pos_world = 0;
	v2		size_world = 1;
	flt		rot = 0;

	////
	bool	black_bars = false; // false

	////
	flt		clip_near = -1;
	flt		clip_far = 100;

	//// movement
	bool	controllable = true; // camera controllable with mouse and keyboard

	flt		base_speed = 0.2f; // in size_world.y / sec
	flt		fast_mult = 4;
	flt		rot_vel = deg(90);
	flt		zoom_log_speed = 0.1f;

	Camera2D () {}
	Camera2D (v2 pos_world, fv2 size_world=1, flt rot=0, bool controllable=true, bool black_bars=false):
		pos_world{pos_world}, size_world{size_world}, rot{rot}, controllable{controllable}, black_bars{black_bars} {}

	int		drag_button = GLFW_MOUSE_BUTTON_RIGHT;
private:
	bool	dragging = false;
	v2		grab_pos_world;
public:

	//
	hm		world_to_cam;
	m4		cam_to_clip;

	hm calc_cam_to_world () {
		return translateH(v3(pos_world,0)) * hm(rotate2(rot));
	}
	hm calc_world_to_cam () {
		return hm(rotate2(-rot)) * translateH(-v3(pos_world,0));
	}

	static Camera2D arcade_style_cam (v2 size_world) {
		return Camera2D((v2)size_world / 2, (v2)size_world, 0, false, true);
	}

private:
	Screen_Rect _screen_rect;
public:

	void update (Input& inp, flt dt, cstr gui_name="Camera2D") {
		update(inp, dt, inp.get_window_screen_rect(), gui_name);
	}

	void update (Input& inp, flt dt, Screen_Rect const& screen_rect, cstr gui_name="Camera2D") {
		
		this->_screen_rect = screen_rect;

		if (imgui::TreeNodeEx(gui_name, ImGuiTreeNodeFlags_CollapsingHeader|(0 ? ImGuiTreeNodeFlags_DefaultOpen : 0))) {
		
			imgui::DragFloat2("pos_world", &pos_world.x, 1.0f / 100);
			imgui::SliderAngle("rot", &rot, -180,+180);
			imgui::DragFloat2("size_world", &size_world.x, 1.0f / 100);

			imgui::Checkbox("black_bars", &black_bars);

			imgui::DragFloat("clip_near", &clip_near, 1.0f / 100);
			imgui::DragFloat("clip_far", &clip_far, 1.0f / 100);

			imgui::Checkbox("controllable", &controllable);

			imgui::DragFloat("base_speed", &base_speed, 1.0f / 10);
			imgui::DragFloat("fast_mult", &fast_mult, 1.0f / 10);
			imgui::DragAngle("rot_vel", &rot_vel, 1.0f / 10);
			imgui::DragFloat("zoom_log_speed", &zoom_log_speed, 1.0f / 10);
		}
		
		if (!black_bars) // never display with wrong aspect ratio
			size_world.x = size_world.y * ((flt)screen_rect.size_px.x / (flt)screen_rect.size_px.y);

		if (controllable) {
			{ // rotation
				int roll_dir = 0;
				if (inp.is_down('Q')) roll_dir -= 1;
				if (inp.is_down('E')) roll_dir += 1;

				rot += (flt)-roll_dir * rot_vel * dt;
				rot = mod_range(rot, deg(-180),deg(+180));
			}
			{ // zooming
				flt size_log = log2(size_world.y); // zoom in log space based on height
				size_log -= inp.get_mousewheel_delta() * zoom_log_speed;

				v2 old_size_world = size_world;

				size_world.y = powf(2, size_log);
				size_world.x = size_world.y * (old_size_world.x / old_size_world.y); // keep aspect ratio
			}
			{ // dragging
				v2 mouse_pos_cam = inp.mouse_cursor_pos_ndc(get_subrect()) * (size_world / 2);

				if (inp.went_down(drag_button) && inp.mouse_curson_in(screen_rect)) {
					grab_pos_world = (rotate2(rot) * mouse_pos_cam) +pos_world;
					dragging = true;
				}

				if (!inp.is_down(drag_button))
					dragging = false;

				if (dragging)
					pos_world = grab_pos_world -(rotate2(rot) * mouse_pos_cam);
			}
			{ // key movement
				iv2 move_dir = 0;
				if (inp.is_down('S'))	move_dir.y -= 1;
				if (inp.is_down('W'))	move_dir.y += 1;
				if (inp.is_down('A'))	move_dir.x -= 1;
				if (inp.is_down('D'))	move_dir.x += 1;

				hm cam_to_world = calc_cam_to_world();

				flt speed = size_world.y * base_speed * (inp.is_down(GLFW_KEY_LEFT_SHIFT) ? fast_mult : 1);

				pos_world += cam_to_world.m2() * (normalize_or_zero((v2)move_dir) * speed) * dt;
			}
		}
	}

	void calc_matricies (Screen_Rect const& rect) {
		world_to_cam = calc_world_to_cam();
		cam_to_clip = engine::calc_orthographic_matrix(size_world, clip_near, clip_far);
	}

	Screen_Rect get_subrect () {
		Screen_Rect subrect = _screen_rect;

		if (black_bars) {
			draw_to_screen(_screen_rect);
			clear(black);

			v2 aspect = size_world;

			v2 tmp = (v2)_screen_rect.size_px / aspect;
			tmp = MIN(tmp.x,tmp.y);

			subrect.size_px = (iv2)(tmp * aspect);
			subrect.offs_px = (_screen_rect.size_px -subrect.size_px) / 2 + _screen_rect.offs_px;
		}

		return subrect;
	}

	void draw_to () {
		Screen_Rect subrect = get_subrect();

		draw_to_screen(subrect);
		calc_matricies(subrect);

		set_shared_uniform("view", "world_to_cam", world_to_cam.m4());
		set_shared_uniform("view", "cam_to_clip", cam_to_clip);
	}
};

}
