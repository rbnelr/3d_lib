
#include "3d_lib/engine.hpp"
#include "3d_lib/camera2D.hpp"
namespace imgui = ImGui;
using namespace engine;

#include "mylibs/random.hpp"

Input* _inp; // only for debugging

struct Snake_Renderer {

	Texture2D atlas = upload_texture_from_file("snake.png", { PF_SRGBA8, USE_MIPMAPS, FILTER_NEAREST, BORDER_CLAMP });
	iv2 atlas_size = iv2(4,4); // in sprites

	iv2 atlas_head[2] =			{ iv2(0,3), iv2(3,1) };
	iv2 atlas_tail =			  iv2(2,0);
	iv2 atlas_body_horiz[3] =	{ iv2(1,3), iv2(1,2), iv2(1,0) };
	iv2 atlas_body_vert[3] =	{ iv2(0,1), iv2(1,1), iv2(2,1) };
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

	void draw_snake_head (iv2 pos, iv2 dir, bool dead) {
		draw_snake_sprite_rot(pos, atlas_head[dead ? 1 : 0], dir);
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

	struct Snake;

	struct Apple {
		iv2		pos = -1;

		bool exists () { return pos.x != -1; }

		static Apple spawn_in_random_pos (Snake& snake, World& world);
	};

	struct Snake {
		struct Body_Piece {
			iv2	pos;
		};

		bool dead = false;

		std::vector<Body_Piece>	body = { // [0]: head   [last]: tail
			{ iv2(5, 5) },
			{ iv2(6, 5) },
			{ iv2(7, 5) },
		};

		iv2 move_dir = iv2(-1,0);

		iv2 get_head_dir () {
			auto& head_pos = body[0].pos;

			iv2 towards_tail = body[1].pos;

			iv2 head_dir = head_pos -towards_tail;
			return head_dir;
		}

		void draw (Snake_Renderer& renderer) {
			for (int i=(int)body.size() -1; i>=0; --i) { // When overlapping the head is on top of the body
				
				auto& pos = body[i].pos;

				iv2* towards_head = i > 0 ?							&body[i -1].pos : nullptr;
				iv2* towards_tail = i < (int)body.size() -1 ?		&body[i +1].pos : nullptr;

				iv2 dir_towards_head = towards_head ? *towards_head -pos : 0;
				iv2 dir_from_tail = towards_tail ? pos -*towards_tail : 0;

				if (i == 0) { // head

					renderer.draw_snake_head(pos, dir_from_tail, dead);

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
		bool collide_body () {
			// head always moves into a place before rest of body, so only check collision between head and rest of body
			for (int i=1; i<(int)body.size(); ++i) {
				if (all(body[0].pos == body[i].pos))
					return true;
			}
			return false;
		}
		
		void set_move_dir (iv2 dir) {
			if (dot(get_head_dir(), dir) == -1) {
				// cannot move backwards (would be moving into body)
				return; // ignore input
			}
			move_dir = dir;
		}

		void move (World& world, Apple& apple, bool has_won) {
			if (dead)
				return; // dont move
			
			iv2 tail_pos = body.back().pos;

			if (has_won)
				move_dir = tail_pos -body[0].pos;

			if (collide_walls(move_dir, world)) { // check wall collision before moving, so when losing head will not be in wall
				dead = true;
				return;
			}

			for (int i=(int)body.size() -1; i>=1; --i) {
				body[i].pos = body[i -1].pos;
			}

			body[0].pos += move_dir;

			if (apple.exists() && all(body[0].pos == apple.pos)) {
				// grow snake
				body.push_back({ tail_pos });

				// spawn apple after growing snake, or snake always dies after winning
				apple = Apple::spawn_in_random_pos(*this, world);
			}

			if (collide_body()) { // check body collision after moving, so when losing head will be over body
				dead = true;
				return;
			}
		}

		void update_cells_have_snake (World* world) {
			for (int i=0; i<(world->snake_cells.x * world->snake_cells.y); ++i)
				world->cells[i].has_snake = false;

			for (auto b : body) {
				world->get_cell(b.pos).has_snake = true;
			}
		}
	};

	World world = World(10);
	Snake snake = 0 ? get_test_snake() : Snake();
	Apple apple = Apple::spawn_in_random_pos(snake, world);

	bool has_won () {
		return !apple.exists(); // could not spawn apple -> won
	}

	void init () {
		_inp = &this->inp;
	}
	void game_tick () {
		
		snake.move(world, apple, has_won());
	}

	struct Game_Timer {
		flt tick_interval = 1.0f / 3;
		flt next_tick_timer = tick_interval;

		flt elapsed = 0;
		int ticks_count = 0;

		void restart () {
			next_tick_timer = tick_interval;

			elapsed = 0;
			ticks_count = 0;
		}

		void imgui () {
			if (imgui::CollapsingHeader("Game_Timer", ImGuiTreeNodeFlags_DefaultOpen)) {

				flt tick_frequency = 1.0f / tick_interval;
				if (imgui::DragFloat("tick_frequency", &tick_frequency, 0.1f))
					tick_interval = 1.0f / tick_frequency;

				imgui::DragFloat("tick_interval", &tick_interval, 0.001f);
				imgui::Value("next_tick_timer", next_tick_timer);

				imgui::Value("elapsed", elapsed);
				imgui::Value("ticks_count", ticks_count);
			}
		}
		bool tick (flt dt) {
			next_tick_timer -= dt;
			elapsed += dt;

			bool do_tick = next_tick_timer <= 0;

			if (do_tick) {
				ticks_count++;

				next_tick_timer += tick_interval;
			}

			return do_tick;
		}
	};

	Game_Timer timer;

	void update_game () {
		
		iv2 inp_dir = 0;
		if ( inp.went_down('A') || inp.went_down(GLFW_KEY_LEFT)	 ) inp_dir = iv2(-1, 0);
		if ( inp.went_down('D') || inp.went_down(GLFW_KEY_RIGHT) ) inp_dir = iv2(+1, 0);
		if ( inp.went_down('S') || inp.went_down(GLFW_KEY_DOWN)	 ) inp_dir = iv2(0, -1);
		if ( inp.went_down('W') || inp.went_down(GLFW_KEY_UP)	 ) inp_dir = iv2(0, +1);
		assert(inp_dir.x == 0 || inp_dir.y == 0);

		imgui::TextColored(ImVec4(0,1,0,1), "SNAKE");

		if (snake.dead) // snake? snake!? SNAAAAKE!
			imgui::TextColored(ImVec4(1,0,0,1), "Game over");
		else if (has_won())
			imgui::TextColored(ImVec4(0,1,0,1), "WON!");
		else
			imgui::Text("");

		if (imgui::Button("Respawn") || inp.went_down('R')) {
			snake = Snake();
			apple = Apple::spawn_in_random_pos(snake, world);

			timer.restart();
		}
		
		imgui::TextColored(ImVec4(0,1,0,1), "Snake size: %d", (int)snake.body.size());

		if (any(inp_dir != 0)) {
			snake.set_move_dir(inp_dir);
		}

		timer.imgui();
		if (!snake.dead && timer.tick(dt))
			game_tick();
	}

	void draw_game () {
		static Camera2D cam = [&] (iv2 size_world) {
			Camera2D cam;

			cam.size_world = (v2)size_world;
			cam.pos_world = (v2)size_world / 2;

			cam.controllable = false;
			cam.black_bars = true;

			return cam;
		} (world.snake_cells);

		//draw_to_screen(inp.wnd_size_px);
		//clear(white);

		//cam.pos_world = (v2)snake.body[0].pos + 0.5f;

		imgui::Separator();

		cam.update(inp, dt);
		cam.draw_to(inp, dt);

		clear(black);

		static Snake_Renderer renderer;

		// draw bg
		for (int y=0; y<world.snake_cells.y; ++y) {
			for (int x=0; x<world.snake_cells.x; ++x) {
				renderer.draw_bg_tile(iv2(x,y));
			}
		}

		// draw apple
		if (apple.exists())
			renderer.draw_apple(apple.pos);

		// draw snake
		snake.draw(renderer);

	}

	void frame () {
		update_game();
		draw_game();
	}

	//
	Snake get_test_snake () {
		char test_snake[10][10] = {
			//'-','>','>','>','>','v',' ',' ',' ',' ',
			//' ',' ','v','<','<','<',' ','>','v',' ',
			//' ',' ','v',' ',' ',' ',' ','^','v',' ',
			//' ',' ','>','v',' ',' ','>','^','v',' ',
			//' ',' ','v','<',' ',' ','^',' ','v',' ',
			//' ',' ','>','>','>','>','^',' ','v',' ',
			//' ',' ',' ',' ',' ',' ',' ',' ','v',' ',
			//' ',' ',' ',' ',' ',' ',' ',' ','O',' ',
			//' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',
			//' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',

			'>','>','>','>','>','>','>','>','>','v',
			'^','v','<','<','<','<','<','<','<','<',
			'^','>','>','>','>','>','>','>','>','v',
			'^','v','<','<','<','<','<','<','<','<',
			'^','>','>','>','>','>','>','>','>','v',
			'-','v','<','<','<','<','<','<','<','<',
			' ','>','>','>','>','>','>','>','>','v',
			' ','v','<','<','<','<','<','<','<','<',
			' ','>','>','>','>','>','>','>','>','v',
			'O','<','<','<','<','<','<','<','<','<',

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
		iv2 tail_dir = iv2(0,+1);
		iv2 head_dir = iv2(0,+1);

		iv2 tail = -1;

		for (int y=0; y<10; ++y) {
			for (int x=0; x<10; ++x) {
				if (test_snake[y][x] == '-') {
					tail = iv2(x,y);
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
				case '-':	return tail_dir * iv2(1,-1);

				default:	return iv2(0);
			}
		};

		iv2 cur = tail;

		Snake snake;
		snake.body.clear();

		for (;;) {
			Snake::Body_Piece p;
			p.pos = cur * iv2(1,-1) +iv2(0,9);
			snake.body.insert(snake.body.begin(), std::move(p));

			auto body = get_snake_body(cur);
			if (body == 'O')
				break; // cur is head

			cur += get_snake_dir(body);
		}
		
		snake.move_dir = head_dir;

		return snake;
	}

} app;

Snake_Game::Apple Snake_Game::Apple::spawn_in_random_pos (Snake& snake, World& world) {
	snake.update_cells_have_snake(&world);

	std::vector<iv2> snakeless_cells;

	for (int y=0; y<world.snake_cells.y; ++y) {
		for (int x=0; x<world.snake_cells.x; ++x) {
			if (!world.get_cell(iv2(x,y)).has_snake) {
				snakeless_cells.push_back(iv2(x,y));
			}
		}
	}

	Apple apple;

	if (snakeless_cells.size() == 0) {
		return apple;
	}

	apple.pos = snakeless_cells[ random::uniform((int)snakeless_cells.size()) ];
	return apple;
}

int main () {
	app.open(MSVC_PROJECT_NAME, iv2(500,500), VSYNC_ON);
	app.init();
	app.run();
	return 0;
}
