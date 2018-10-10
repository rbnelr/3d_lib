
#include "3d_lib/engine.hpp"
#include "3d_lib/camera2D.hpp"
namespace imgui = ImGui;
using namespace engine;

#include "stdlib.h"
#include "time.h"

// Tetris pieces (Tetronimos):

// I piece:		X
//				X
//				X
//				X

// O piece:		XX
//				XX

// T piece:		XXX
//				 X

// S piece:		X
//				XX
//				 X

// Z piece:		 X
//				XX
//				X

// J piece:		 X
//				 X
//				XX

// L piece:		X
//				X
//				XX

struct Tetromino_Type {
	std::string			name;
	std::vector<iv2>	blocks;
	lrgb				col;
};

lrgb orange_red	= srgb8(237, 102,   0).to_lrgb();
lrgb light_blue	= srgb8( 99, 203, 255).to_lrgb();
lrgb gold		= srgb8(255, 207,  35).to_lrgb();
lrgb blue		= srgb8(  0,  29, 249).to_lrgb();
lrgb magenta	= srgb8(249,   0, 190).to_lrgb();
lrgb green		= srgb8( 29, 249,   0).to_lrgb();
lrgb yellow		= srgb8(236, 249,   0).to_lrgb();

std::vector<Tetromino_Type> tetromino_types = {
	{ "I", { iv2(0,+1), iv2(0,0), iv2(0,-1), iv2(0,-2)	}, orange_red	},
	{ "O", { iv2(0,0), iv2(+1,0), iv2(0,-1), iv2(+1,-1)	}, light_blue	},
	{ "T", { iv2(0,0), iv2(-1,0), iv2(+1,0), iv2(0,+1)	}, gold			},
	{ "S", { iv2(0,+1), iv2(0,0), iv2(+1,0), iv2(+1,-1)	}, blue			},
	{ "Z", { iv2(0,+1), iv2(0,0), iv2(-1,0), iv2(-1,-1)	}, magenta		},
	{ "J", { iv2(0,+1), iv2(0,0), iv2(0,-1), iv2(-1,-1)	}, green		},
	{ "L", { iv2(0,+1), iv2(0,0), iv2(0,-1), iv2(+1,-1)	}, yellow		},
};

struct Block {
	Tetromino_Type*	type = nullptr; // nullptr is background
};

iv2 tetris_visible_cells = iv2(10,20);
iv2 tetris_stored_cells = tetris_visible_cells +iv2(0,2);

struct Placed_Blocks {
	std::vector<Block> cells = std::vector<Block> (tetris_stored_cells.x * tetris_stored_cells.y);
	
	Block* get (iv2 pos_world) {
		if (any(pos_world < 0 || pos_world >= tetris_stored_cells))
			return nullptr;
		return &cells[pos_world.y * tetris_stored_cells.x + pos_world.x];
	}
	void draw () {

		iv2 pos;
		for (pos.y=0; pos.y<tetris_visible_cells.y; ++pos.y) {
			for (pos.x=0; pos.x<tetris_visible_cells.x; ++pos.x) {
				auto* block = get(pos);

				if (block->type) {
					draw_rect((v2)pos +0.5f, 1, lrgba(block->type->col, 1));
				}
			}
		}
	}

	void update (int* score) {
		std::vector<int> rows_to_remove;

		iv2 pos = 0;
		for (pos.y=0; pos.y<tetris_stored_cells.y; ++pos.y) {
			bool full_row = true;
			for (pos.x=0; pos.x<tetris_stored_cells.x; ++pos.x) {
				if (get(pos)->type == nullptr)
					full_row = false;
			}

			if (full_row)
				rows_to_remove.push_back(pos.y);
		}

		for (int i=(int)rows_to_remove.size()-1; i>=0; --i) {
			int y = rows_to_remove[i];

			for (pos.y=y; pos.y<tetris_stored_cells.y; ++pos.y) {
				for (pos.x=0; pos.x<tetris_stored_cells.x; ++pos.x) {
					auto* top = get(pos +iv2(0,+1));
					get(pos)->type = top ? top->type : nullptr;
				}
			}

			(*score)++;
		}
	}
};

flt base_move_interval = 1.7f;
flt move_fast_multiplier = 6;
flt speedup = 1;

flt move_timer = base_move_interval;

struct Active_Tetromino {
	Tetromino_Type*	type;
	iv2				pos_world;
	int				ori = 0;

	bool is_active () {
		return type != nullptr;
	}

	std::vector<iv2> blocks_rotated_world;

	void draw () {

		for (iv2 pos : blocks_rotated_world) {
			draw_rect((v2)pos +0.5f, 1, lrgba(type->col, 1));
		}
	}

	iv2 rotate (iv2 p, int ori) {
		return rotate2_90(p, ori);
	}

	void rotate_blocks (int ori) {
		blocks_rotated_world = type->blocks;

		for (auto& b : blocks_rotated_world)
			b = rotate(b, ori) + pos_world;
	}

	bool check_blocks_overlap (Placed_Blocks* blocks, iv2 pos_world, int ori) {
		for (iv2 b : type->blocks) {
			b = rotate(b, ori) + pos_world;
			if (!blocks->get(b) || blocks->get(b)->type != nullptr)
				return true;
		}
		return false;
	}
	void place_tetronimo (Placed_Blocks* blocks) {
		for (auto& b : blocks_rotated_world) {
			if (!(blocks->get(b) && blocks->get(b)->type == nullptr))
				errprint("blocks->get(b) && blocks->get(b)->type == nullptr");
			//assert(blocks->get(b) && blocks->get(b)->type == nullptr);
			if (blocks->get(b))
				blocks->get(b)->type = type;
		}
		type = nullptr;
	}

	void update (Input& inp, flt dt, Placed_Blocks* blocks) {
		bool move = false;

		if (move_timer <= 0) {
			move = true;
			move_timer = base_move_interval;
		}

		bool move_fast = inp.is_down('S');
		int move_dir = 0;
		move_dir -= inp.went_down_repeat('A') ? 1 : 0;
		move_dir += inp.went_down_repeat('D') ? 1 : 0;

		move_timer -= dt * speedup * (move_fast ? move_fast_multiplier : 1);

		if (!check_blocks_overlap(blocks, pos_world +iv2(move_dir,0), ori)) {
			pos_world += iv2(move_dir,0);
		}

		iv2 dropped_pos = pos_world + iv2(0,-1);

		if (move && !check_blocks_overlap(blocks, dropped_pos, ori)) {
			pos_world += iv2(0,-1); 
		}

		int rot = 0;
		if (inp.went_down('Q'))	rot += 1;
		if (inp.went_down('E'))	rot -= 1;

		{
			auto new_ori = ori + rot;
			new_ori = (int)((uint)new_ori % 4);

			rotate_blocks(new_ori);

			if (!check_blocks_overlap(blocks, dropped_pos, new_ori)) {
				ori = new_ori;
			}
		}

		if (move && check_blocks_overlap(blocks, dropped_pos, ori)) {
			place_tetronimo(blocks);
		}
	}
};

Active_Tetromino spawn_tetromino (Tetromino_Type* type) {
	Active_Tetromino t;
	t.type = type;
	t.pos_world = iv2(tetris_visible_cells.x / 2, 18);
	return t;
}
Active_Tetromino spawn_random_tetromino () {
	return spawn_tetromino( &tetromino_types[ rand() % (int)tetromino_types.size() ] );
}

struct App : public Application {
	void frame () {

		static Camera2D cam = Camera2D::arcade_style_cam((v2)tetris_visible_cells);

		cam.update(inp, dt);
		cam.draw_to();

		static int score = 0;

		static auto bg_color = srgb8(7,14,32).to_lrgb();
		clear(bg_color);

		speedup = pow(1.05f, score);

		static Placed_Blocks		blocks;
		static Active_Tetromino		active_tetromino = spawn_random_tetromino();
		if (inp.went_down('R') || !active_tetromino.is_active())
			active_tetromino = spawn_random_tetromino();

		if (active_tetromino.is_active())
			active_tetromino.update(inp, dt, &blocks);

		blocks.update(&score);
		blocks.draw();

		imgui::Text("Score: %d", score * 10);
		imgui::Text("Speed: %.2f", speedup);

		if (active_tetromino.is_active())
			active_tetromino.draw();

	}
};

int main () {
	App app;
	app.open(MSVC_PROJECT_NAME);
	app.run();
	return 0;
}
