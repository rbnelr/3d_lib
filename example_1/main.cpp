
#include "3d_lib/engine.hpp"
using namespace float_precision;
namespace imgui = ImGui;
using namespace colors;
using namespace math;
using namespace vector;

using engine::Input;

void game (Input& inp) {
	iv2 tetris_visible_cells = iv2(10,20);

	iv2 frame_size_px;
	{
		engine::draw_to_screen(inp.wnd_size_px);
		engine::clear(0);

		v2 tetris_aspect = (v2)tetris_visible_cells;

		// black bars
		v2 tmp = (v2)inp.wnd_size_px / tetris_aspect;
		tmp = min(tmp.x,tmp.y);

		frame_size_px = (iv2)(tmp * tetris_aspect);

		iv2 frame_offs = (inp.wnd_size_px -frame_size_px) / 2;

		engine::draw_to_screen(frame_offs, frame_size_px);
	}
	static auto bg_color = srgb8(7,14,32).to_lrgb();
	engine::clear(bg_color);

	static v2 cam_offs=0, cam_sz=0;
	imgui::DragFloat2("cam_offs", &cam_offs.x, 1.0f / 30);
	imgui::DragFloat2("cam_sz", &cam_sz.x, 1.0f / 30);

	hm world_to_cam = translateH(v3( -((v2)tetris_visible_cells / 2 +cam_offs), 0));
	m4 cam_to_clip = engine::calc_orthographic_matrix(((v2)tetris_visible_cells + cam_sz) / 2);

	engine::set_shared_uniform("view", "world_to_cam", world_to_cam.m4());
	engine::set_shared_uniform("view", "cam_to_clip", cam_to_clip);

	static iv2 pos=iv2(4, 12);
	imgui::DragInt2("pos", &pos.x, 1.0f / 20);

	for (auto p : { iv2(-1,0), iv2(0,0), iv2(+1,0), iv2(0,-1) })
		engine::draw_rect((v2)(pos +p) +0.5f, 1, 1);
}

int main () {
	
	engine::Application app;
	app.open(MSVC_PROJECT_NAME);

	Delta_Time_Measure dt_measure;
	flt dt = dt_measure.begin();

	for (int frame_i=0;; ++frame_i) {
		app.poll_input();
		if (app.wants_to_close()) break;

		if (app.inp.went_down(GLFW_KEY_F11))
			app.toggle_fullscreen();

		engine::begin_imgui(&app.inp, dt);

		game(app.inp);

		engine::draw_to_screen(app.inp.wnd_size_px);
		engine::end_imgui(app.inp.wnd_size_px);

		app.swap_buffers();

		dt = dt_measure.frame();
	}

	return 0;
}
