
#include "3d_lib/engine.hpp"
#include "3d_lib/camera.hpp"
#include "mylibs/random.hpp"
using namespace engine;

#include "mylibs/intersect.hpp"
using namespace intersect;

class Block {
public:
	enum type_e {
		EMPTY	=0,
		WOOD,

		TYPES_COUNT
	};

	type_e type = EMPTY;
};

class Ship {
public:
	
	std::string			name;

	v3					pos_world;
	quat				ori_world;

	iv3					size;
	unique_ptr<Block[]> blocks; // 3d blocks array of size

	// model coord space: 0 is center of ship
	// voxel coord space: block[0][0][0] goes from (0,0,0) to (1,1,1)

	Gpu_Mesh			mesh;

	bool				dead = false;

	static Block* _index (unique_ptr<Block[]> const& blocks, iv3 pos, iv3 size) {
		assert(all(pos >= 0 && pos < size));
		return blocks.get()	+ pos.z * size.x * size.y
							+ pos.y * size.x
							+ pos.x;
	}
	Block* get_block (iv3 pos) const {
		return _index(blocks, pos, size);
	}
	void allocate_blocks (iv3 new_size) {
		blocks = make_unique<Block[]>( new_size.z * new_size.y * new_size.x );
		size = new_size;
	}

	template <typename FOREACH> void interate_3d (iv3 sz, FOREACH foreach) {
		iv3 p;
		for (p.z=0; p.z<sz.z; ++p.z)
			for (p.y=0; p.y<sz.y; ++p.y)
				for (p.x=0; p.x<sz.x; ++p.x)
					foreach(p);
	}

	void place_block (iv3 pos) {
		if (!all(pos >= 0 && pos < size)) { // find out if we need to grow voxel grid
			iv3 min = 0;
			iv3 max = size -1;

			iv3 old_max = max;

			min = MIN(min, pos);
			max = MAX(max, pos);

			iv3 blocks_offset = -min;

			iv3 old_size = size;
			iv3 new_size = max -min +1;

			auto old_blocks = std::move(blocks);

			allocate_blocks(new_size);

			interate_3d(old_size, [&] (iv3 p) {
				*get_block(p + blocks_offset) = *_index(old_blocks, p, old_size);
			});

			pos += blocks_offset;


			v3 ship_center_offset = (v3)(min +(max -old_max)) * 0.5f;
			pos_world += ship_center_offset;
		}

		get_block(pos)->type = Block::WOOD;

		remesh();
	}
	void delete_block (iv3 pos) {
		
		get_block(pos)->type = Block::EMPTY;

		// find out if we can shrink voxel grid
		iv3 min = size -1;
		iv3 max = 0;
		{
			interate_3d(size, [&] (iv3 p) {
				if (get_block(p)->type != Block::EMPTY) {
					min = MIN(min, p);
					max = MAX(max, p);
				}	
			});
		}

		iv3 old_max = size -1;

		iv3 blocks_offset = -min;

		iv3 old_size = size;
		iv3 new_size = max -min +1;

		auto old_blocks = std::move(blocks);

		allocate_blocks(new_size);

		interate_3d(new_size, [&] (iv3 p) {
			*get_block(p) = *_index(old_blocks, p -blocks_offset, old_size);
		});

		v3 ship_center_offset = (v3)(min +(max -old_max)) * 0.5f;
		pos_world += ship_center_offset;

		if (all(new_size == 1) && get_block(0)->type == Block::EMPTY)
			dead = true;

		remesh();
	}

	v3 voxel_pos_to_model (v3 pos_voxel) {
		return pos_voxel -(v3)size / 2;
	}
	v3 model_pos_to_voxel (v3 pos_model) {
		return pos_model +(v3)size / 2;
	}

	void remesh () {
		Cpu_Mesh<Default_Vertex_3d, u32> cpu_mesh;

		interate_3d(size, [&] (iv3 p) {
			if (get_block(p)->type != Block::EMPTY)
				cpu_mesh.add(Default_Vertex_3d::gen_cube( voxel_pos_to_model((v3)p) +0.5f ));
		});

		mesh = cpu_mesh.upload();
	}

	bool raycast (v3 ray_pos, v3 ray_dir, iv3* hit_block=0, v3* hit_pos=0, iv3* hit_face_normal=0) {
		
		hm model_to_world = translateH(pos_world) * convert_to_hm(ori_world);
		hm world_to_model = translateH(-pos_world) * convert_to_hm(inverse(ori_world));

		ray_pos = world_to_model * ray_pos;
		ray_dir = world_to_model.m3() * ray_dir;

		flt hit_t, exit_t;

		if (!intersect_AABB(ray_pos, 1 / ray_dir, -(v3)size/2, +(v3)size/2, &hit_t, &exit_t))
			return false;
		
		v3 ship_hit_pos = ray_pos + ray_dir * hit_t;
		flt intersect_length = exit_t -hit_t;

		auto raycast_voxel = [&] (iv3 block_pos, v3 hit_pos, int hit_face) {
			if (!all(block_pos >= 0 && block_pos < size))
				return false;

			return get_block(block_pos)->type != Block::EMPTY;
		};

		ray_pos = model_pos_to_voxel(ship_hit_pos);

		int hit_face;
		if (!raycast_voxels(raycast_voxel, ray_pos, ray_dir, intersect_length, hit_block, hit_pos, &hit_face))
			return false;

		if (hit_face_normal) *hit_face_normal = face_normals[hit_face];

		return true;
	}

	static unique_ptr<Ship> new_empty_ship () {
		static int counter = 0;

		auto s = make_unique<Ship>();

		s->name = prints("Ship %d", counter++);

		s->pos_world = 0;
		s->ori_world = 0 ? rotateQ_Z(deg(30)) : quat::ident(); // TODO: Fix rotation

		s->allocate_blocks(iv3(1,1,1));

		s->get_block(0)->type = Block::WOOD;

		s->remesh();

		return s;
	}

	bool	highlight_block;
	iv3		highlight_block_pos;

	void build_update (Input& inp, flt dt, Camera const& cam) {
		
		auto ray = cam.get_mouse_ray_world(inp);
		
		iv3 hit_block;
		v3 hit_pos;
		iv3 face_normal;

		bool mouse_hit = raycast(ray.pos, ray.dir, &hit_block, &hit_pos, &face_normal);
		highlight_block = mouse_hit;
		if (mouse_hit) {

			if (inp.went_down(GLFW_MOUSE_BUTTON_LEFT))
				place_block(hit_block + face_normal);
			else if (inp.went_down(GLFW_MOUSE_BUTTON_RIGHT))
				delete_block(hit_block);

			highlight_block_pos = hit_block + face_normal;
		}

		if (inp.is_down('M')) {
			v3 mirror_pos;

			if (highlight_block) {
				mirror_pos = (v3)highlight_block_pos + round(hit_pos * 2); // round to .0 and .5
			} else {
				mirror_pos = (v3)size / 2;
			}

			hm world_to_model = translateH(-pos_world) * convert_to_hm(inverse(ori_world));

			int mirror_axis = biggest_comp(world_to_model * cam.forw_dir_world());
			
			v3 size = 0.05f;

			size[mirror_axis] = 0.95f;

			draw_box_outline(pos_world + mirror_pos, quat::ident(), size, lrgba(0.5f,0.5f,1,1));
		}
	}

	void update (Input& inp, flt dt, Camera const& cam) {
		
		if (imgui::TreeNodeEx(prints("%s###%p", name.c_str(), this).c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
			
			imgui::InputText_str("name", &name);
			imgui::Value("size", size);
			
			imgui::DragFloat3("pos_world", &pos_world.x, 1.0f / 20);

			if (imgui::Button("delete"))
				dead = true;

			imgui::TreePop();
		}

		build_update(inp, dt, cam);
	}

	void draw () {

		draw_box_outline(pos_world, quat::ident(), (v3)size, lrgba(1,0,0,1));
		
		draw_simple(mesh, pos_world, ori_world, v3(1), srgb8(122,71,36).to_lrgba());

		if (highlight_block)
			draw_box_outline(pos_world + voxel_pos_to_model((v3)highlight_block_pos) + 0.5f, quat::ident(), 0.99f, lrgba(0.5f, 1, 0.5f, 1));
	}
};

class Ship_Manager {
public:
	
	std::vector<unique_ptr<Ship>> ships;

	void update (Input& inp, flt dt, Camera const& cam) {
		
		if (imgui::Button("New Ship"))
			ships.emplace_back( Ship::new_empty_ship() );

		for (auto s = ships.begin(); s!=ships.end();) {
			(*s)->update(inp, dt, cam);

			if ((*s)->dead)
				s = ships.erase(s);
			else
				s++;
		}
	}
	void draw () {
		for (auto& s : ships)
			s->draw();
	}
};

class App : public Application {

	Ship_Manager ships;
	
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

		ships.update(inp, dt, cam);
		ships.draw();
	}
} app;

int main () {
	app.open(MSVC_PROJECT_NAME);
	app.run();
	return 0;
}
