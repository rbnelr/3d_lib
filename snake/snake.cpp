
#include "3d_lib/engine.hpp"
#include "3d_lib/camera2D.hpp"
namespace imgui = ImGui;
using namespace engine;

#include "mylibs/random.hpp"

struct Arcade_Camera : Camera2D {
	// view has fixed aspect ratio -> centered with black bars
	
	iv2 view_size;

	Arcade_Camera (iv2 view_size): view_size{view_size} {
		key_reset = 'T';
		fixed = true;
		base_vsize_world = (flt)view_size.y;
		pos_world = (v2)view_size / 2;
	}

	void update_viewport (Input& inp, flt dt) {
		draw_to_screen(inp.wnd_size_px);
		clear(0);

		v2 aspect = (v2)view_size;

		// black bars
		v2 tmp = (v2)inp.wnd_size_px / aspect;
		tmp = min(tmp.x,tmp.y);

		iv2 frame_size_px = (iv2)(tmp * aspect);

		iv2 frame_offs = (inp.wnd_size_px -frame_size_px) / 2;

		draw_to_screen(frame_offs, frame_size_px);

		update(inp, dt, { frame_offs, frame_size_px });

		set_shared_uniform("view", "world_to_cam", world_to_cam.m4());
		set_shared_uniform("view", "cam_to_clip", cam_to_clip);
	}
};

struct Snake_Renderer {

	Texture2D atlas = upload_texture_from_file("snake.png", { PF_SRGBA8, USE_MIPMAPS, FILTER_NEAREST, BORDER_CLAMP });
	iv2 atlas_size = iv2(4,4); // in sprites

	iv2 atlas_head =				iv2(0,3);
	iv2 atlas_tail =				iv2(2,0);
	iv2 atlas_body_horiz[3] =	{ iv2(1,3), iv2(1,2), iv2(1,0) };
	iv2 atlas_body_vert[3] =		{ iv2(0,1), iv2(1,1), iv2(2,1) };
	iv2 atlas_body_curve[4] =	{ iv2(2,3), iv2(2,2), iv2(0,2), iv2(0,0) };

	void draw_snake_sprite_rot (iv2 pos, iv2 tile, iv2 tile_dir=iv2(-1,0)) {

		int rot;
		if (		tile_dir.x == -1 )
			rot = 0;
		else if (	tile_dir.y == -1 )
			rot = 1;
		else if (	tile_dir.x == +1 )
			rot = 2;
		else /*if (tile_dir.y == +1 )*/
			rot = 3;

		draw_sprite((v2)pos, v2(1), rot, tile, atlas_size, atlas);
	}

	void draw_snake_head (iv2 pos, iv2 dir) {
		draw_snake_sprite_rot(pos, atlas_head, dir);
	}
	void draw_snake_tail (iv2 pos, iv2 dir) {
		draw_snake_sprite_rot(pos, atlas_tail, dir);
	}

	void draw_snake_mid_piece (iv2 pos, iv2 dir_from_tail, iv2 dir_towards_head, int body_piece_i) {

		bool is_straight_body = all(dir_from_tail == dir_towards_head);

		iv2 tile;

		if (is_straight_body) { // straight body piece

			bool is_horiz_body = dir_towards_head.x != 0;
			if (is_horiz_body)
				tile = atlas_body_horiz[body_piece_i % 3];
			else
				tile = atlas_body_vert[body_piece_i % 3];

		} else { // corner body piece

			if (	(dir_towards_head - dir_from_tail).x == -1 ) {

				if ((dir_towards_head - dir_from_tail).y == -1)
					tile = atlas_body_curve[0];
				else
					tile = atlas_body_curve[1];
			} else {

				if ((dir_towards_head - dir_from_tail).y == -1)
					tile = atlas_body_curve[2];
				else
					tile = atlas_body_curve[3];
			}

		}

		draw_snake_sprite_rot(pos, tile);
	}

	void draw_apple (iv2 pos) {
		static iv2 atlas_apple =			iv2(3,3);

		draw_sprite((v2)pos, v2(1), 0, atlas_apple, atlas_size, atlas);
	}

	void draw_bg_tile (iv2 pos) {
		static iv2 atlas_bg_tile =			iv2(3,2);

		int random_rot = (std::hash<int>()(pos.x) * 13 + std::hash<int>()(pos.y) * 17) % 4;

		draw_sprite((v2)pos, v2(1), random_rot, atlas_bg_tile, atlas_size, atlas);
	}
};

struct Snake_Game : engine::Application {
	
	struct World {
		iv2 snake_cells;

		struct Cell {
			bool has_snake = false;
		};
		unique_ptr<Cell[]> cells;

		World (iv2 snake_cells): snake_cells{snake_cells} {
			cells = make_unique<Cell[]>(snake_cells.x * snake_cells.y);
		}
	
		bool in_world (iv2 pos) {
			return all(pos >= 0 && pos < snake_cells);
		}

		Cell& get_cell (iv2 pos) {
			assert(in_world(pos));

			return cells[pos.y * snake_cells.x + pos.x];
		}
	};

	struct Snake {
		struct Body_Piece {
			iv2	pos;
		};

		std::vector<Body_Piece>	body; // [0]: head   [last]: tail

		iv2 move_dir;

		static Snake initial_snake () {
			Snake s;

			s.body = {
				{ iv2(5, 5) },
				{ iv2(6, 5) },
				{ iv2(7, 5) },
			};

			s.move_dir = iv2(-1,0);

			return s;
		}

		iv2 get_head_dir () {
			auto& head_pos = body[0].pos;

			iv2 towards_tail = body[1].pos;

			iv2 head_dir = head_pos -towards_tail;
			return head_dir;
		}

		void draw (Snake_Renderer& renderer) {
			for (int i=0; i<(int)body.size(); ++i) {
				
				auto& pos = body[i].pos;

				iv2* towards_head = i > 0 ?							&body[i -1].pos : nullptr;
				iv2* towards_tail = i < (int)body.size() -1 ?		&body[i +1].pos : nullptr;

				iv2 dir_towards_head = towards_head ? *towards_head -pos : 0;
				iv2 dir_from_tail = towards_tail ? pos -*towards_tail : 0;

				if (i == 0) { // head
					
					renderer.draw_snake_head(pos, dir_from_tail);

				} else if (i == ((int)body.size() -1)) { // tail

					renderer.draw_snake_tail(pos, dir_towards_head);

				} else { // mid body piece

					renderer.draw_snake_mid_piece(pos, dir_from_tail, dir_towards_head, i);
				}
			}
		}

		bool collide_walls (iv2 dir, World& world) {
			return !world.in_world(body[0].pos + dir);
		}
		
		void set_move_dir (iv2 dir) {
			if (dot(get_head_dir(), dir) == -1) {
				// cannot move backwards (would be moving into body)
				return; // ignore input
			}
			move_dir = dir;
		}

		void move (bool grow_snake, World& world) {
			if (collide_walls(move_dir, world)) {
				return; // ignore for now
			}

			if (grow_snake)
				body.push_back({});

			for (int i=(int)body.size() -1; i>=1; --i) {
				body[i].pos = body[i -1].pos;
			}

			body[0].pos += move_dir;
		}

		void update_cells_have_snake (World* world) {
			for (int i=0; i<(world->snake_cells.x * world->snake_cells.y); ++i)
				world->cells[i].has_snake = false;

			for (auto b : body) {
				world->get_cell(b.pos).has_snake = true;
			}
		}
	};

	struct Apple {
		iv2		pos = -1;

		bool exists () { return pos.x != -1; }

		random::Uniform_Int rand;

		std::vector<iv2> snakeless_cells;

		void respawn (Snake& snake, World& world) {
			snake.update_cells_have_snake(&world);

			snakeless_cells.clear();

			for (int y=0; y<world.snake_cells.y; ++y) {
				for (int x=0; x<world.snake_cells.x; ++x) {
					if (!world.get_cell(iv2(x,y)).has_snake) {
						snakeless_cells.push_back(iv2(x,y));
					}
				}
			}

			if (snakeless_cells.size() == 0) {
				// win
				pos = -1;
				return;
			}

			pos = snakeless_cells[ rand.roll((int)snakeless_cells.size()) ];
		}
	};

	World world = World(10);
	Snake snake;
	Apple apple;

	void init () {
		//snake = get_test_snake(&apple);
		snake = Snake::initial_snake();

		apple.respawn(snake, world);
	}
	void game_tick () {
		
		static bool grow_snake = false;

		snake.move(grow_snake, world);
		grow_snake = false;

		if (apple.exists() && all(snake.body[0].pos == apple.pos)) {
			apple.respawn(snake, world);
			grow_snake = true;
		}
	}
	void update_game () {
		
		iv2 inp_dir = 0;
		if ( inp.went_down('A') || inp.went_down(GLFW_KEY_LEFT)	 ) inp_dir = iv2(-1, 0);
		if ( inp.went_down('D') || inp.went_down(GLFW_KEY_RIGHT) ) inp_dir = iv2(+1, 0);
		if ( inp.went_down('S') || inp.went_down(GLFW_KEY_DOWN)	 ) inp_dir = iv2(0, -1);
		if ( inp.went_down('W') || inp.went_down(GLFW_KEY_UP)	 ) inp_dir = iv2(0, +1);
		assert(inp_dir.x == 0 || inp_dir.y == 0);

		if (any(inp_dir != 0)) {
			snake.set_move_dir(inp_dir);
		}

		//
		static flt tick_interval = 1.0f / 3;

		static flt next_tick_timer = tick_interval;

		next_tick_timer -= dt;

		if (next_tick_timer <= 0) {
			game_tick();

			next_tick_timer += tick_interval;
		}
	}

	void draw_game () {
		static Arcade_Camera cam (world.snake_cells);
		cam.update_viewport(inp, dt);

		static Snake_Renderer renderer;

		// draw bg
		for (int y=0; y<world.snake_cells.y; ++y) {
			for (int x=0; x<world.snake_cells.x; ++x) {
				renderer.draw_bg_tile(iv2(x,y));
			}
		}

		// draw snake
		snake.draw(renderer);

		// draw apple
		if (apple.exists())
			renderer.draw_apple(apple.pos);
	}

	void frame () {
		update_game();
		draw_game();
	}

	//
	Snake get_test_snake (Snake_Game::Apple* apple) {
		char test_snake[10][10] = {
			'-','>','>','>','>','v',' ',' ',' ',' ',
			' ',' ','v','<','<','<',' ','>','v',' ',
			' ',' ','v',' ',' ',' ',' ','^','v',' ',
			' ',' ','>','v',' ',' ','>','^','v',' ',
			' ',' ','v','<',' ',' ','^',' ','v',' ',
			' ',' ','>','>','>','>','^',' ','v',' ',
			' ',' ',' ',' ',' ',' ',' ',' ','v',' ',
			' ',' ',' ',' ',' ',' ',' ',' ','O',' ',
			' ',' ',' ',' ','A',' ',' ',' ',' ',' ',
			' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',

			//'-','>','>','>','>','>','>','>','>','v',
			//' ',' ',' ',' ',' ',' ',' ',' ',' ','v',
			//' ',' ',' ',' ',' ',' ',' ',' ',' ','v',
			//' ',' ',' ',' ',' ',' ',' ',' ',' ','v',
			//' ',' ',' ',' ',' ',' ',' ',' ',' ','v',
			//' ',' ',' ',' ',' ',' ',' ',' ',' ','v',
			//' ',' ',' ',' ',' ',' ',' ',' ',' ','v',
			//' ',' ',' ',' ',' ',' ',' ',' ',' ','v',
			//' ',' ',' ',' ',' ',' ',' ',' ',' ','v',
			//' ',' ',' ',' ',' ',' ',' ',' ',' ','O',
		};

		iv2 tail = -1;

		for (int y=0; y<10; ++y) {
			for (int x=0; x<10; ++x) {
				if (test_snake[y][x] == '-') {
					tail = iv2(x,y);
				}
				if (test_snake[y][x] == 'A') {
					Snake_Game::Apple a;
					a.pos = iv2(x,y) * iv2(1,-1) +iv2(0,9);
					*apple = std::move(a);
				}
			}
		}

		if (tail.x == -1)
			return {};

		auto get_snake_body = [&] (iv2 pos) {
			if (any(pos < 0 || pos >= 10))	return '\0';
			return test_snake[pos.y][pos.x];
		};
		auto get_snake_dir = [&] (char body) {
			switch (body) {
				case '>':	return iv2(+1,0);
				case '<':	return iv2(-1,0);
				case 'v':	return iv2(0,+1);
				case '^':	return iv2(0,-1);
				case '-':	return iv2(+1,0);

				default:	return iv2(0);
			}
		};

		iv2 cur = tail;

		Snake snake;

		for (;;) {
			Snake::Body_Piece p;
			p.pos = cur * iv2(1,-1) +iv2(0,9);
			snake.body.insert(snake.body.begin(), std::move(p));

			auto body = get_snake_body(cur);
			if (body == 'O')
				break; // cur is head

			cur += get_snake_dir(body);
		}

		return snake;
	}

} app;

int main () {
	app.open(MSVC_PROJECT_NAME, iv2(500,500), VSYNC_ON);
	app.init();
	app.run();
	return 0;
}
