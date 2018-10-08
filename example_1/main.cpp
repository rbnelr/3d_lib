
#include "3d_lib/engine.hpp"
using namespace float_precision;
namespace imgui = ImGui;
using namespace colors;
using namespace math;
using namespace vector;

#include "3d_lib/camera2D.hpp"

using engine::Input;
using engine::Camera2D;

void game (Input& inp) {

	iv2 tetris_visible_cells = iv2(10,20);

	static Camera2D cam = Camera2D::arcade_style_cam((v2)tetris_visible_cells);

	cam.update(inp, 1.0f / 60);
	cam.draw_to();

	static auto bg_color = srgb8(7,14,32).to_lrgb();
	engine::clear(bg_color);

	static iv2 pos=iv2(4, 12);
	imgui::DragInt2("pos", &pos.x, 1.0f / 20);

	for (auto p : { iv2(-1,0), iv2(0,0), iv2(+1,0), iv2(0,-1) })
		engine::draw_rect((v2)(pos +p) +0.5f, 1, 1);
}

int main () {
	
	engine::Window wnd;
	wnd.open(MSVC_PROJECT_NAME);

	Delta_Time_Measure dt_measure;
	flt dt = dt_measure.begin();

	for (int frame_i=0;; ++frame_i) {
		wnd.poll_input();
		if (wnd.wants_to_close()) break;

		if (wnd.inp.went_down(GLFW_KEY_F11))
			wnd.toggle_fullscreen();

		engine::begin_imgui(&wnd.inp, dt);

		game(wnd.inp);

		engine::draw_to_screen(wnd.inp.wnd_size_px);
		engine::end_imgui(wnd.inp.wnd_size_px);

		wnd.swap_buffers();

		dt = dt_measure.frame();
	}

	return 0;
}
