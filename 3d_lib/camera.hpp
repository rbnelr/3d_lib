#pragma once

#include "engine_include.hpp"
#include "dear_imgui.hpp"

namespace engine {

struct Camera {
	v3		pos_world = 0;
	v3		altazimuth = 0; // azimuth elevation roll

	flt		vfov = deg(75);
	flt		clip_near = 1.0f / 16;
	flt		clip_far = 1024;

	// flycam movement
	flt		base_speed = 1;
	flt		fast_mult = 4;

	void update (Input& inp, flt dt) {

		if (imgui::TreeNodeEx("Camera", ImGuiTreeNodeFlags_CollapsingHeader|(0 ? ImGuiTreeNodeFlags_DefaultOpen : 0))) {

			imgui::DragFloat3("pos_world", &pos_world.x, 1.0f / 100);
			imgui::DragFloat3("altazimuth", &altazimuth.x, 1.0f / 100);

			imgui::SliderAngle("vfov", &vfov, 0,180);

			imgui::DragFloat("clip_near", &clip_near, 1.0f / 100);
			imgui::DragFloat("clip_far", &clip_far, 1.0f / 100);

			imgui::DragFloat("base_speed", &base_speed, 1.0f / 10);
			imgui::DragFloat("fast_mult", &fast_mult, 1.0f / 10);
		}
		{
			save->value("pos_world", &pos_world);
			save->angle("altazimuth", &altazimuth);

			save->angle("vfov", &vfov);

			save->value("clip_near", &clip_near);
			save->value("clip_far", &clip_far);

			save->value("base_speed", &base_speed);
			save->value("fast_mult", &fast_mult);
		}

		flt roll_vel = deg(45); // ang / sec

		int roll_dir = 0;
		//if (inp.is_down('Q')) roll_dir -= 1;
		//if (inp.is_down('E')) roll_dir += 1;

		altazimuth.z += (flt)-roll_dir * roll_vel * dt;

		v2 mouselook_sensitivity = deg(0.12f); // ang / mouse_move_px

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

		flt speed = base_speed * (inp.is_down(GLFW_KEY_LEFT_SHIFT) ? fast_mult : 1);

		pos_world += cam_to_world.m3() * (normalize_or_zero((v3)move_dir) * speed) * dt;

		calc_matricies(inp);
	}

	hm calc_cam_to_world () {
		quat rot = rotateQ_Z(altazimuth.x) * rotateQ_X(altazimuth.y +deg(90)) * rotateQ_Z(altazimuth.z); // camera starts looking at -z with camera up being +y
		return translateH(pos_world) * hm(convert_to_m3(rot));
	}
	hm calc_world_to_cam () {
		quat rot = rotateQ_Z(-altazimuth.z) * rotateQ_X(-altazimuth.y -deg(90)) * rotateQ_Z(-altazimuth.x); // rotate world around camera
		return hm(convert_to_m3(rot)) * translateH(-pos_world);
	}

	hm		world_to_cam;
	m4		cam_to_clip;

	void calc_matricies (Input& inp) {
		world_to_cam = calc_world_to_cam();

		flt aspect_wh = (flt)inp.wnd_size_px.x / (flt)inp.wnd_size_px.y;
		cam_to_clip = engine::calc_perspective_matrix(vfov, clip_near, clip_far, aspect_wh);
	}
};

}
