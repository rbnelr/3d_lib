
#include "3d_lib/engine.hpp"
#include "3d_lib/camera.hpp"
#include "mylibs/random.hpp"
using namespace engine;

#include "mylibs/intersect.hpp"
using namespace intersect;

struct Block {
	enum type_e {
		EMPTY	=0,
		WOOD,

		TYPES_COUNT
	};

	type_e type;

	bool hit;
};

struct Ship {
	
	static constexpr iv3 size = iv3(16,32,16);
	Block blocks[size.z][size.y][size.x];

	// model coord space: 0 is center of ship
	// voxel coord space: block[0][0][0] goes from (0,0,0) to (1,1,1)

	static v3 voxel_pos_to_model (v3 pos_voxel) {
		return pos_voxel -(v3)size / 2;
	}
	static v3 model_pos_to_voxel (v3 pos_model) {
		return pos_model +(v3)size / 2;
	}

	Gpu_Mesh mesh;

	v3		pos_world;
	quat	ori_world;

	bool raycast (v3 ray_pos, v3 ray_dir, v3* out_hit_pos=0, v3* out_face_normal=0) {
		
		for (int z=0; z<size.z; ++z) {
			for (int y=0; y<size.y; ++y) {
				for (int x=0; x<size.x; ++x) {
					blocks[z][y][x].hit = false;
				}
			}
		}

		hm model_to_world = translateH(pos_world) * convert_to_hm(ori_world);
		hm world_to_model = translateH(-pos_world) * convert_to_hm(inverse(ori_world));

		ray_pos = world_to_model * ray_pos;
		ray_dir = world_to_model.m3() * ray_dir;

		flt hit_t, exit_t;

		if (!intersect_AABB(ray_pos, 1 / ray_dir, -(v3)size/2, +(v3)size/2, &hit_t, &exit_t))
			return false;
		
		v3 ship_hit_pos = ray_pos + ray_dir * hit_t;

		draw_box_outline(ship_hit_pos, 0.1f, lrgba(0,1,0,1));

		auto raycast_block = [&] (iv3 block_pos, v3 hit_pos, v3 face_normal) {
			assert(all(block_pos >= 0 && block_pos < size));
			
			auto& b = blocks[block_pos.z][block_pos.y][block_pos.x];

			b.hit = true;

			return true;
		};

		raycast_voxels(raycast_block, model_pos_to_voxel(ship_hit_pos), ray_dir, iv3(0),size);

		return true;
	}

	static Ship gen_test_player_ship () {
		Ship s;
		s.pos_world = 0;
		s.ori_world = quat::ident();

		for (int z=0; z<size.z; ++z) {
			for (int y=0; y<size.y; ++y) {
				for (int x=0; x<size.x; ++x) {
					s.blocks[z][y][x].type = (Block::type_e)random::uniform((int)Block::TYPES_COUNT);
				}
			}
		}

		// generate mesh
		Cpu_Mesh<Default_Vertex_3d, u32> cpu_mesh;

		for (int z=0; z<size.z; ++z) {
			for (int y=0; y<size.y; ++y) {
				for (int x=0; x<size.x; ++x) {
					if (s.blocks[z][y][x].type != Block::EMPTY)
						cpu_mesh.add(Default_Vertex_3d::gen_cube( voxel_pos_to_model((v3)iv3(x,y,z)) ));
				}
			}
		}

		s.mesh = cpu_mesh.upload();

		return s;
	}

	void update (Input& inp, flt dt, Camera const& cam) {

		if (inp.went_down('B'))
			int a = 5;

		static Camera::Raycast_Result ray;
		static bool freeze_ray = false;
		if (inp.went_down('F'))
			freeze_ray = !freeze_ray;

		if (!freeze_ray)
			ray = cam.get_mouse_ray_world(inp);

		draw_line(ray.pos, ray.pos + ray.dir * 500, lrgba(1,0,0,1));

		if (raycast(ray.pos, ray.dir))
			imgui::Text("HIT!!");
	}

	void draw () {
		
		//draw_simple(mesh, pos_world, ori_world, v3(1), srgb8(122,71,36).to_lrgba());

		for (int z=0; z<size.z; ++z) {
			for (int y=0; y<size.y; ++y) {
				for (int x=0; x<size.x; ++x) {
					if (blocks[z][y][x].hit) {
						draw_box_outline( voxel_pos_to_model((v3)iv3(x,y,z)) +0.5f, 1, lrgba(0,1,0,1));
					}
				}
			}
		}

		draw_box_outline(pos_world, (v3)size, lrgba(1,0,0,1));
	}
};

struct App : public Application {

	Ship player_ship;
	bool spawn_ship = true;
	
	void frame () {
		
		imgui::Separator();

		static bool wireframe_enable = false;
		save->value("wireframe_enable", &wireframe_enable);
		imgui::Checkbox("wireframe_enable", &wireframe_enable);
		engine::set_shared_uniform("wireframe", "enable", wireframe_enable);

		static Camera cam;

		cam.update(inp, dt);
		cam.draw_to();

		//
		engine::draw_to_screen(inp.wnd_size_px);
		engine::clear(0);

		draw_skybox_gradient();

		if (spawn_ship) {
			player_ship = Ship::gen_test_player_ship();
			spawn_ship = false;
		}

		player_ship.update(inp, dt, cam);
		player_ship.draw();
	}
} app;

int main () {
	app.open(MSVC_PROJECT_NAME);
	app.run();
	return 0;
}
