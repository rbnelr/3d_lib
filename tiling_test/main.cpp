
#include "3d_lib/engine.hpp"
#include "3d_lib/camera2D.hpp"
#include "mylibs/random.hpp"

using namespace engine;
using namespace imgui;

std::vector<v2> points;
Cpu_Mesh<Vertex_Draw_Lines> lines;

// Gen points
flt factor = 0.617f;
int max_depth = 3;
flt angle_deg = 20;

void recurse_gen_points (v2 pos, v2 dir, flt angle, flt size, int depth, int prev_dir=0) {
	flt ang_a = angle -angle_deg * factor;
	flt ang_b = angle +angle_deg * factor;
	flt ang_c = angle * (1 + factor);

	v2 dir_a = rotate2(deg(+90 +ang_a)) * dir;
	v2 dir_b = rotate2(deg(-90 +ang_b)) * dir;
	v2 dir_c = rotate2(deg(ang_c)) * dir;

	v2 pos_a = pos + dir_a * size * factor;
	v2 pos_b = pos + dir_b * size * factor;
	v2 pos_c = pos + dir_c * size * factor;

	//points.push_back(pos_c);

	if (depth == max_depth) return;

	size *= factor;

	if (prev_dir != -1) {
		recurse_gen_points(pos_a, dir_a, ang_a, size, depth +1, -1);

		lines.vertecies.push_back({pos});
		lines.vertecies.push_back({pos_a});
	}
	if (prev_dir != +1) {
		recurse_gen_points(pos_b, dir_b, ang_b, size, depth +1, +1);

		lines.vertecies.push_back({pos});
		lines.vertecies.push_back({pos_b});
	}
	{
		recurse_gen_points(pos_c, dir_c, ang_c, size, depth +1, 0);
		lines.vertecies.push_back({pos});
		lines.vertecies.push_back({pos_c});
	}
}
void gen_points () {
	static flt size = 1;
	
	imgui::DragFloat("size", &size, 1.0f / 50);
	imgui::DragInt("max_depth", &max_depth, 1.0f / 50);
	imgui::DragFloat("factor", &factor, 1.0f / 30);
	imgui::DragFloat("angle", &angle_deg, 1.0f / 3);

	points.clear();
	lines.clear();
	factor;

	//points.push_back(0);

	recurse_gen_points(0, v2(0,+1), 0, size, 0);
}

struct App : public Application {
	void frame () {

		static bool wireframe_enable = false;
		save->value("wireframe_enable", &wireframe_enable);
		imgui::Checkbox("wireframe_enable", &wireframe_enable);
		engine::set_shared_uniform("wireframe", "enable", wireframe_enable);

		static Camera2D cam (0, 5.1f);

		cam.update(inp, dt);
		cam.draw_to();

		Cpu_Mesh<Default_Vertex_3d> mesh;

		//
		engine::draw_to_screen(inp.wnd_size_px);
		engine::clear(0.1f);

		if (0) {
			for (int y=-5; y<6; ++y) {
				for (int x=-5; x<6; ++x) {
					points.push_back((v2)iv2(x,y));
				}
			}
		} else {
			gen_points();
		}

		for (auto p : points)
			draw_rect(p, 0.05f, lrgba(0,0,0,1));
		draw_lines(lines, 0, 1, lrgba(1,0.5f,0.5f,1));
	}
};

int main () {
	App app;
	app.open(MSVC_PROJECT_NAME);
	app.run();
	return 0;
}

