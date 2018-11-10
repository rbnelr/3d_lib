#pragma once

#include "engine_include.hpp"
#include "dear_imgui.hpp"

namespace engine {

class Camera {
public:
	////
	v3		pos_world = 0;
	v3		altazimuth = 0; // azimuth elevation roll

	////
	flt		vfov = deg(75);
	flt		clip_near = 1.0f / 16;
	flt		clip_far = 1024;

	//// flycam movement
	bool	controllable = true; // camera controllable with mouse and keyboard

	flt		base_speed = 1;
	flt		fast_mult = 4;
	
	v2		mouselook_sensitivity = deg(0.12f); // ang / mouse_move_px
	flt		roll_vel = 1 ? 0 : deg(45); // ang / sec

	flt		speedup_log_speed = 0.2f;

	////
	hm		world_to_cam;
	m4		cam_to_clip;

	hm		cam_to_world;
	m4		clip_to_cam;

	Camera () {}
	Camera (v3 pos_world, v3 altazimuth=0, bool controllable=true):
		pos_world{pos_world}, altazimuth{altazimuth}, controllable{controllable} {}


	v3 ndc_to_cam (v2 ndc, flt z_cam) const {
		
		flt w_clip = -z_cam;

		v2 clip = ndc * w_clip;

		v2 cam = v2(clip_to_cam.arr[0].x,clip_to_cam.arr[1].y) * clip;

		return v3(cam, z_cam);
	}

	struct Raycast_Result {
		v3 pos;
		v3 dir;
	};
	Raycast_Result get_mouse_ray_cam (Input& inp) const {
		Raycast_Result r = {};

		v2 ndc = inp.mouse_cursor_pos_ndc(_screen_rect);
		v3 cam = ndc_to_cam(ndc, -clip_near);

		r.pos = cam;
		r.dir = normalize(cam);

		return r;
	}
	Raycast_Result get_mouse_ray_world (Input& inp) const {
		auto r = get_mouse_ray_cam(inp);

		r.pos = cam_to_world * r.pos;
		r.dir = normalize(cam_to_world.m3() * r.dir);

		return r;
	}

	void update (Input& inp, flt dt, cstr gui_name="Camera") {
		update(inp, dt, inp.get_window_screen_rect(), gui_name);
	}

	void update (Input& inp, flt dt, Screen_Rect const& screen_rect, cstr gui_name="Camera") {

		this->_screen_rect = screen_rect;

		if (imgui::TreeNodeEx(gui_name, ImGuiTreeNodeFlags_CollapsingHeader|(0 ? ImGuiTreeNodeFlags_DefaultOpen : 0))) {

			imgui::DragFloat3("pos_world", &pos_world.x, 1.0f / 100);
			imgui::DragFloat3("altazimuth", &altazimuth.x, 1.0f / 100);

			{
				flt deg = to_deg(vfov);
				imgui::SliderFloat("vfov", &deg, 0,180, "%.1f", 3);
				vfov = to_rad(deg);
			}

			imgui::DragFloat("clip_near", &clip_near, 1.0f / 100);
			imgui::DragFloat("clip_far", &clip_far, 1.0f / 100);

			imgui::DragFloat("base_speed", &base_speed, 1.0f / 10);
			imgui::DragFloat("fast_mult", &fast_mult, 1.0f / 10);
		}
		{
			save->begin(gui_name);

			save->value("pos_world", &pos_world);
			save->angle("altazimuth", &altazimuth);

			save->angle("vfov", &vfov);

			save->value("clip_near", &clip_near);
			save->value("clip_far", &clip_far);

			save->value("base_speed", &base_speed);
			save->value("fast_mult", &fast_mult);

			save->end();
		}

		if (controllable) {
			int roll_dir = 0;
			if (inp.is_down('Q')) roll_dir -= 1;
			if (inp.is_down('E')) roll_dir += 1;

			altazimuth.z += (flt)-roll_dir * roll_vel * dt;

			if ((!inp.gui_input_enabled || inp.is_down(GLFW_MOUSE_BUTTON_RIGHT)) && !inp.block_mouse) {
				altazimuth.x -=  inp.mousecursor.delta_screen.x * mouselook_sensitivity.x;
				altazimuth.y += -inp.mousecursor.delta_screen.y * mouselook_sensitivity.y; // delta_screen is top down
			}

			altazimuth.x = mod_range(altazimuth.x, deg(-180), deg(+180));
			altazimuth.y = clamp(altazimuth.y, deg(-90), deg(+90));
			altazimuth.z = mymod(altazimuth.z, deg(360));

			// movement
			iv3 move_dir = 0;
			if (inp.is_down('S'))					move_dir.z += 1;
			if (inp.is_down('W'))					move_dir.z -= 1;
			if (inp.is_down('A'))					move_dir.x -= 1;
			if (inp.is_down('D'))					move_dir.x += 1;
			if (inp.is_down(GLFW_KEY_LEFT_CONTROL))	move_dir.y -= 1;
			if (inp.is_down(' '))					move_dir.y += 1;

			hm cam_to_world = calc_cam_to_world();

			{
				flt log = log2(base_speed);
				log += inp.get_mousewheel_delta() * speedup_log_speed;

				base_speed = powf(2, log);
			}

			flt speed = base_speed * (inp.is_down(GLFW_KEY_LEFT_SHIFT) ? fast_mult : 1);

			pos_world += cam_to_world.m3() * (normalize_or_zero((v3)move_dir) * speed) * dt;

		}
	}

	hm calc_cam_to_world () {
		quat rot = rotateQ_Z(altazimuth.x) * rotateQ_X(altazimuth.y +deg(90)) * rotateQ_Z(altazimuth.z); // camera starts looking at -z with camera up being +y
		return translateH(pos_world) * hm(convert_to_m3(rot));
	}
	hm calc_world_to_cam () {
		quat rot = rotateQ_Z(-altazimuth.z) * rotateQ_X(-altazimuth.y -deg(90)) * rotateQ_Z(-altazimuth.x); // rotate world around camera
		return hm(convert_to_m3(rot)) * translateH(-pos_world);
	}

	void calc_matricies (Screen_Rect const& rect) {
		world_to_cam = calc_world_to_cam();
		cam_to_world = calc_cam_to_world();

		flt aspect_wh = (flt)rect.size_px.x / (flt)rect.size_px.y;
		cam_to_clip = engine::calc_perspective_matrix(vfov, clip_near, clip_far, aspect_wh, &clip_to_cam);
	}

private:
	Screen_Rect _screen_rect;
public:
	
	void draw_to () {
		draw_to_screen(_screen_rect);
		calc_matricies(_screen_rect);

		set_shared_uniform("view", "world_to_cam", world_to_cam.m4());
		set_shared_uniform("view", "cam_to_clip", cam_to_clip);
	}
};

}
