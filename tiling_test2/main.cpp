
#include "3d_lib/engine.hpp"
#include "3d_lib/camera2D.hpp"
#include "mylibs/random.hpp"

using namespace engine;
using namespace imgui;

std::vector<v2> points;
Cpu_Mesh<Vertex_Draw_Lines> lines;

#define JC_VORONOI_IMPLEMENTATION 1
#include "deps/jcash_voronoi/src/jc_voronoi.h"

// Gen points
int points_count = 100;
flt area = 1;
int relaxations = 0;

jcv_diagram clac_voronoi (std::vector<v2> const& points) { // need to jcv_diagram_free(&diagram); !
	jcv_diagram diagram;
	memset(&diagram, 0, sizeof(jcv_diagram));

	jcv_rect rect;
	rect.min.x = -area;
	rect.min.y = -area;
	rect.max.x = +area;
	rect.max.y = +area;

	jcv_diagram_generate((int)points.size(), (jcv_point*)&points[0], &rect, &diagram); // calc voronoi diagram

	return diagram;
}

void voronoi_relax (std::vector<v2>* points) {
	jcv_diagram diagram = clac_voronoi(*points);
	defer {
		jcv_diagram_free(&diagram);
	};

	const jcv_site* cells = jcv_diagram_get_sites(&diagram);

	// relax original points
	for (int cell_i=0; cell_i<diagram.numsites; ++cell_i) {
		auto& cell = cells[cell_i];

		jcv_point sum = cell.p;
		int count = 1;

		auto* edge = cell.edges;
		while (edge) {
			sum.x += edge->pos[0].x;
			sum.y += edge->pos[0].y;
			++count;
			edge = edge->next;
		}

		(*points)[cell.index].x = sum.x / count;
		(*points)[cell.index].y = sum.y / count;
	}
}
void voronoi_draw (std::vector<v2> const& points) {
	jcv_diagram diagram = clac_voronoi(points);
	defer {
		jcv_diagram_free(&diagram);
	};
	
	const jcv_site* cells = jcv_diagram_get_sites(&diagram);

	for (int cell_i=0; cell_i<diagram.numsites; ++cell_i) {
		auto& cell = cells[cell_i];

		auto* edge = cell.edges;
		while (edge) { // draw voronoi cells

			lines.vertecies.push_back({ v2(edge->pos[0].x,edge->pos[0].y) });
			lines.vertecies.push_back({ v2(edge->pos[1].x,edge->pos[1].y) });

			edge = edge->next;
		}
	}
}

void gen_points () {
	imgui::DragInt("points_count", &points_count);
	imgui::DragFloat("area", &area, 1.0f / 30);
	imgui::DragInt("relaxations", &relaxations, 1.0f / 20);

	points.clear();
	lines.clear();

	auto rand = random::Generator(0);

	for (int i=0; i<points_count; ++i) {
		points.push_back( random::uniform(rand, v2(-area),v2(+area)) );
	}

	for (int i=0; i<relaxations; ++i)
		voronoi_relax(&points);
	voronoi_draw(points);
}

struct App : public Application {
	void frame () {
	
		static bool wireframe_enable = false;
		save->value("wireframe_enable", &wireframe_enable);
		imgui::Checkbox("wireframe_enable", &wireframe_enable);
		engine::set_shared_uniform("wireframe", "enable", wireframe_enable);

		static Camera2D cam (0, 2.1f);

		cam.update(inp, dt);
		cam.draw_to();

		Cpu_Mesh<Default_Vertex_3d> mesh;
	
		//
		engine::draw_to_screen(inp.wnd_size_px);
		engine::clear(0.1f);
	
		gen_points();

		for (auto p : points)
			draw_rect(p, 0.01f, lrgba(0,0,0,1));
		draw_lines(lines, 0, 1, lrgba(1,0.5f,0.5f,1));
	}
};

int main () {
	App app;
	app.open(MSVC_PROJECT_NAME);
	app.run();
	return 0;
}
