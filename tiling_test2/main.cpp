
#include "3d_lib/engine.hpp"
#include "3d_lib/camera.hpp"
#include "mylibs/random.hpp"

using namespace engine;
using namespace imgui;

std::vector<v2> points;
Cpu_Mesh<Vertex_Draw_Lines> lines;

#define JC_VORONOI_IMPLEMENTATION 1
#include "deps/jcash_voronoi/src/jc_voronoi.h"

// Gen points
int points_count = 100;
flt area = 4;
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

	random::seed(0);

	for (int i=0; i<points_count; ++i) {
		points.push_back( random::rand_v2(-area,+area) );
	}

	for (int i=0; i<relaxations; ++i)
		voronoi_relax(&points);
	voronoi_draw(points);
}

void frame (Display& dsp, Input& inp, flt dt) {
	
	static bool wireframe_enable = false;
	save->value("wireframe_enable", &wireframe_enable);
	imgui::Checkbox("wireframe_enable", &wireframe_enable);
	engine::set_shared_uniform("wireframe", "enable", wireframe_enable);

	static Camera cam;

	{ // view
		cam.update(inp, dt);

		engine::set_shared_uniform("view", "cam_to_clip", cam.cam_to_clip);
		engine::set_shared_uniform("view", "world_to_cam", cam.world_to_cam.m4());
	}

	Cpu_Mesh<Default_Vertex_3d> mesh;
	
	//
	engine::draw_to_screen(inp.wnd_size_px);
	engine::clear(0);

	draw_skybox_gradient();
	
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

int main () {
	random::seed(0);

	engine::run_display(frame, MSVC_PROJECT_NAME);
	return 0;
}
