
#include "3d_lib/engine.hpp"
#include "3d_lib/camera.hpp"
#include "mylibs/random.hpp"
using namespace engine;
using namespace imgui;

struct Voxel {
	flt		density;
};
struct Voxels {
	iv3					size;
	unique_ptr<Voxel[]>	voxels;

	Voxels (iv3 size): size{size} {
		voxels = make_unique<Voxel[]>(size.z * size.y * size.x);
	}

	Voxel* get (iv3 pos) const {
		assert(all(pos >= 0 && pos < size));
		return voxels.get() + pos.z * size.y * size.x
							+ pos.y * size.x
							+ pos.x;
	}
};

struct Generator {
	bool regen = true;

	iv3	size = iv3(32,32,16);

	bool imgui (Voxels* voxels) {
		save->begin("Generator");
		save->value("size", &size);
		save->end();

		if (!imgui::CollapsingHeader("Generator", ImGuiTreeNodeFlags_DefaultOpen))
			return false;

		regen = imgui::DragInt3("size", &size.x, 1.0f / 20) || regen;

		if (regen) {
			*voxels = gen_voxels();
			regen = false;
			return true;
		}
		return false;
	}

	Voxels gen_voxels () {
		Voxels v = Voxels(size);

		iv3 p;
		for (p.z=0; p.z<size.z; ++p.z) {
			for (p.y=0; p.y<size.y; ++p.y) {
				for (p.x=0; p.x<size.x; ++p.x) {
					v.get(p)->density = 1 -((flt)p.z * 1 + (flt)p.y * 0.2f + (flt)p.x * 0.1f) / 24;
				}
			}
		}

		return v;
	}
};

#include "marching_cubes.hpp"

Gpu_Mesh meshify (Voxels const& voxels, flt isolevel=0.5f) {

	Cpu_Mesh<Default_Vertex_3d> mesh;

	//iv3 start = 0;
	//iv3 end = voxel_area -1;
	iv3 start = -1;
	iv3 end = voxels.size;

	iv3 p;
	for (p.z=start.z; p.z<end.z; ++p.z) {
		for (p.y=start.y; p.y<end.y; ++p.y) {
			for (p.x=start.x; p.x<end.x; ++p.x) {

				using namespace marching_cubes;

				Triangle tris[5];

				Gridcell gridcell;

				iv3 corners[] = {
					iv3(0,1,0),
					iv3(1,1,0),
					iv3(1,0,0),
					iv3(0,0,0),
					iv3(0,1,1),
					iv3(1,1,1),
					iv3(1,0,1),
					iv3(0,0,1),
				};

				for (int i=0; i<8; ++i) {
					iv3 corner_pos = p +corners[i];
					gridcell.p[i] = (v3)corner_pos;

					flt density; // 1: mass  0: air (surface has normals facing air)

					if (all(corner_pos >= 0 && corner_pos < voxels.size))
						density = voxels.get(corner_pos)->density;
					else
						density = 0;

					gridcell.val[i] = density;
				}

				int tri_count = Polygonise(gridcell, isolevel, tris);

				auto vert = [&] (v3 pos, v3 normal) {
					Default_Vertex_3d v;
					v.pos_model = pos;
					v.normal_model = normal;
					mesh.vertecies.push_back(v);
				};

				for (int i=0; i<tri_count; ++i) {
					v3 a = tris[i].p[0];
					v3 b = tris[i].p[1];
					v3 c = tris[i].p[2];

					v3 ab = b -a;
					v3 ac = c -a;

					v3 normal = normalize_or_zero( cross(ab, ac) );

					//if (length(a -b) < 0.01f)
					//	printf("degenerate triangle!\n");
					//if (length(b -c) < 0.01f)
					//	printf("degenerate triangle!\n");
					//if (length(c -a) < 0.01f)
					//	printf("degenerate triangle!\n");
					//if (length(normal) < 0.01f)
					//	printf("normal zero!\n");

					if (length(normal) != 0) {
						vert(a, normal);
						vert(b, normal);
						vert(c, normal);
					}
				}
			}
		}
	}

	return Gpu_Mesh::upload(mesh);
}

struct App : public Application {
	void frame () {
	
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

		static Voxels voxels = Voxels(iv3(32,32,16));
		static Gpu_Mesh mesh;

		static Generator gen;

		static bool regen_voxels = true;
		static flt regen_seconds = -1;

		regen_voxels = gen.imgui(&voxels) || regen_voxels;

		Text("regen_voxels: %8.3f ms", regen_seconds * 1000);

		static flt isolevel = 0.5f;
		regen_voxels = imgui::DragFloat("isolevel", &isolevel, 1.0f / 30) || regen_voxels;

		if (regen_voxels) {
			Timer t;
			t.start();

			mesh = meshify(voxels, isolevel);

			regen_seconds = t.end();
		}
		regen_voxels = false;

		Text("voxels: %d", voxels.size.z * voxels.size.y * voxels.size.x);
		Text("mesh: %8d %8d -> %9d bytes", mesh.vertex_count, mesh.index_count, mesh.layout->vertex_size * mesh.vertex_count + mesh.index_count * mesh.get_index_size_bytes(mesh.index_type));

		//
		engine::draw_to_screen(inp.wnd_size_px);
		engine::clear(0);

		draw_skybox_gradient();

		draw_simple(mesh, 0);
	}
} app;

int main () {
	app.open(MSVC_PROJECT_NAME);
	app.run();
	return 0;
}
