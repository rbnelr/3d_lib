
#include "3d_lib/engine.hpp"
#include "3d_lib/camera.hpp"
#include "mylibs/random.hpp"
using namespace engine;

flt smoothstep_n (flt x, int n) {
	for (int i=0; i<n; ++i) {
		x = smoothstep(x);
	}
	return x;
}

#include "marching_cubes.hpp"

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

		using namespace engine;
		using namespace imgui;

		static flt voxels[16][32][32];
		static Gpu_Mesh blocky;
		static Gpu_Mesh smooth;

		static bool regen_voxels = true;
		static flt regen_seconds = -1;
	
		regen_voxels = Button("regen_voxels") || regen_voxels;
		SameLine();
		Text("%8.3f ms", regen_seconds * 1000);

		static bool fixed_seed = true;
		Checkbox("fixed_seed", &fixed_seed);

		static int current_voxel_example = 0;
		regen_voxels = imgui::Combo("voxel_example", &current_voxel_example,
				"rand_50%\0"
				"rand_20%\0"
				"rand_gradient_linear%\0"
				"rand_gradient_pow%\0"
				"rand_gradient_smoothstep%\0"
				"rand_gradient_sphere%\0"
				"rand_gradient_torus%\0"
			) || regen_voxels;
		save->value("current_voxel_example", &current_voxel_example);

		static flt settings[] = {
			0.5f,
			0.2f,
			-1,
			1.5f,
			0.7f,
			8.14f,
			4.5f,
		};

		regen_voxels = imgui::DragFloat("setting", &settings[current_voxel_example], 1.0f / 300) || regen_voxels;

		flt setting = settings[current_voxel_example];

		static bool binary_density = true;
		regen_voxels = Checkbox("binary_density", &binary_density) || regen_voxels;

		if (regen_voxels) {
			Timer t;
			t.start();

			static random::Generator gen;

			if (fixed_seed)
				gen = random::Generator(0);

			iv3 voxel_area = iv3(32,32,16);

			auto voxelize = [&] (flt (*func)(v3 pos, float setting), flt setting) {
				iv3 p;
				for (p.z=0; p.z<voxel_area.z; ++p.z) {
					for (p.y=0; p.y<voxel_area.y; ++p.y) {
						for (p.x=0; p.x<voxel_area.x; ++p.x) {
							flt tmp = clamp( func( (v3)p, setting ), 0.0f,1.0f);
							if (binary_density)
								tmp = tmp >= 0.5f ? 1.0f : 0.0f;
							voxels[p.z][p.y][p.x] = tmp;
						}
					}
				}
			};
			auto rand = [&] (float percent_filled) {
				voxelize([] (v3 pos, float percent_filled) -> flt {
					flt prob = percent_filled;
					return map(random::uniform(gen, 0.0f,1.0f), prob -0.05f, prob +0.05f); 
				}, percent_filled);
			};
			//auto rand_gradient = [&] (float (*gradient)(v3 pos, float setting)) {
			//	voxelize([] (v3 pos, float setting) -> flt {
			//		flt prob = gradient((v3)p, setting);
			//		return map(random::rand_float(0,1), prob -0.05f, prob +0.05f); 
			//	}, setting);
			//};
			auto gradient = [&] (float (*dens_gradient)(v3 pos, float setting)) {
				voxelize(dens_gradient, setting);
			};

			switch (current_voxel_example) {
				case 0:
					rand(setting);
					break;
				case 1:
					rand(setting);
					break;
				case 2:
					//rand_gradient([] (v3 pos, float setting) -> flt { return 1 -(pos.z / 15); });
					break;
				case 3:
					//rand_gradient([] (v3 pos, float setting) -> flt { return pow(1 -(pos.z / 15),setting); });
					break;
				case 4:
					//rand_gradient([] (v3 pos, float setting) -> flt { return smoothstep_n(1 -(pos.z / 15), (int)lerp(1, 10, setting)); });
					break;
				case 5:
					gradient([] (v3 pos, float setting) -> flt {
						flt r = length(pos -v3(16,16,8));
						flt sphere_r = setting;
						return map(r, sphere_r +0.5f, sphere_r -0.5f); 
					});
					break;
				case 6:
					gradient([] (v3 pos, float setting) -> flt {
						v3 torus_center = v3(16,16,8);
						flt torus_radius_ring = 12;
						flt torus_radius_inner = setting;

						flt dist_r = length( length(pos.xy() -torus_center.xy()) -torus_radius_ring);
						flt dist_h = length(pos.z -torus_center.z);

						flt r = length(v2(dist_r,dist_h));

						return map(r, torus_radius_inner +0.5f, torus_radius_inner -0.5f); 
					});
					break;
				default:
					break;
			}

			auto meshify_blocky = [&] () {
				Cpu_Mesh<Default_Vertex_3d,GLuint> cpu_mesh;

				for (int z=0; z<voxel_area.z; ++z) {
					for (int y=0; y<voxel_area.y; ++y) {
						for (int x=0; x<voxel_area.x; ++x) {

							v3 pos_world = (v3)iv3(x,y,z);

							if (voxels[z][y][x] >= 0.5f) {
								auto cube = gen_cube<engine::Default_Vertex_3d>([] (v3 p, v3 n, v2 uv, int face) {
									Default_Vertex_3d v;
									v.pos_model = p;
									v.normal_model = n;
									v.uv = uv;
									return v;
								}, 0.5f, pos_world);

								cpu_mesh.add(cube);
							}
						}
					}
				}

				return Gpu_Mesh::upload(cpu_mesh);
			};

			auto meshify_smooth = [&] () {
			
				Cpu_Mesh<Default_Vertex_3d> mesh;

				//iv3 start = 0;
				//iv3 end = voxel_area -1;
				iv3 start = -1;
				iv3 end = voxel_area;

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

								if (all(corner_pos >= 0 && corner_pos < voxel_area))
									density = (flt)voxels[corner_pos.z][corner_pos.y][corner_pos.x];
								else
									density = 0;

								gridcell.val[i] = 1 -density;
							}

							int tri_count = Polygonise(gridcell, 0.5f, tris);

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

								if (length(a -b) < 0.01f)
									printf("degenerate triangle!\n");
								if (length(b -c) < 0.01f)
									printf("degenerate triangle!\n");
								if (length(c -a) < 0.01f)
									printf("degenerate triangle!\n");
								if (length(normal) < 0.01f)
									printf("normal zero!\n");

								vert(a, normal);
								vert(b, normal);
								vert(c, normal);
							}
						}
					}
				}

				return Gpu_Mesh::upload(mesh);
			};

			blocky = meshify_blocky();
			smooth = meshify_smooth();

			regen_seconds = t.end();
		}
		regen_voxels = false;

		//
		engine::draw_to_screen(inp.wnd_size_px);
		engine::clear(0);

		draw_skybox_gradient();

		static bool show_smooth = true;
		imgui::Checkbox("show_smooth", &show_smooth);

		draw_simple(show_smooth ? smooth : blocky, 0);
	}
} app;

int main () {
	app.open(MSVC_PROJECT_NAME);
	app.run();
	return 0;
}
