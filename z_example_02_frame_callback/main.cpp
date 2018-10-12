
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

lrgb orange_red	= lrgb(1);
lrgb light_blue	= lrgb(1);
lrgb gold		= lrgb(1);
lrgb blue		= lrgb(1);
lrgb magenta	= lrgb(1);
lrgb green		= lrgb(1);
lrgb yellow		= lrgb(1);

std::vector<Tetromino_Type> tetromino_types = {
	{ "I", { iv2(0,+1), iv2(0,+1), iv2(0,+1), iv2(0,+2)	}, orange_red	},
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
	unique_ptr<Block[]> cells = make_unique<Block[]>(tetris_stored_cells.x * tetris_stored_cells.y);

	void draw () {
		
		iv2 pos;
		for (pos.y=0; pos.y<tetris_visible_cells.y; ++pos.y) {
			for (pos.x=0; pos.x<tetris_visible_cells.x; ++pos.x) {
				auto block = cells.get()[pos.y * tetris_stored_cells.x + pos.x];

				if (block.type) {
					draw_rect((v2)pos +0.5f, 1, lrgba(block.type->col, 1));
				}
			}
		}
	}
};

flt move_interval = 0.25f;
flt move_fast_multiplier = 4;

flt move_timer = move_interval;

struct Active_Tetromino {
	Tetromino_Type*	type;
	iv2				pos_world;
	int				ori = 0;

	std::vector<iv2> blocks_rotated_world;

	void draw () {
		
		for (iv2 pos : blocks_rotated_world) {
			draw_rect((v2)pos +0.5f, 1, lrgba(type->col, 1));
		}
	}

	iv2 rotate (iv2 p) {
		return rotate2_90(p, ori);
	}

	void rotate_blocks () {
		blocks_rotated_world = type->blocks;

		for (auto& b : blocks_rotated_world)
			b = rotate(b) + pos_world;
	}

	void update (Input& inp, flt dt) {
		bool move = false;

		if (move_timer <= 0) {
			move = true;
			move_timer = move_interval;
		}
		move_timer -= dt;

		if (move) {
			pos_world.y -= 1; 
		}

		int rot = 0;
		if (inp.went_down(GLFW_KEY_LEFT) || inp.went_down(GLFW_KEY_A))	rot += 1;
		if (inp.went_down(GLFW_KEY_RIGHT) || inp.went_down(GLFW_KEY_D))	rot -= 1;

		ori += rot;
		ori = (int)((uint)ori % 4);

		rotate_blocks();
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

struct Tetris : public engine::Application {
	virtual ~Tetris () {}

	void frame () {
	
		static Camera2D cam = Camera2D::arcade_style_cam((v2)tetris_visible_cells);

		cam.update(inp, dt);
		cam.draw_to();

		static auto bg_color = srgb8(7,14,32).to_lrgb();
		clear(bg_color);
	
		static Placed_Blocks		blocks;
		static Active_Tetromino		active_tetromino = spawn_random_tetromino();
		active_tetromino.update(inp, dt);

		blocks.draw();
		active_tetromino.draw();
	}
};

Tetris tetris;

int main () {
	srand((uint)time(NULL));
	tetris.open(MSVC_PROJECT_NAME);
	tetris.run();
	return 0;
}
