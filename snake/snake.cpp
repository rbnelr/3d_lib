
#include "3d_lib/engine.hpp"
#include "3d_lib/camera2D.hpp"
namespace imgui = ImGui;
using namespace engine;

#include "stdlib.h"
#include "time.h"

iv2 snake_cells = 10;

iv2 apple_pos = -1;

std::vector<iv2> get_test_snake () {
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
				apple_pos = iv2(x,y) * iv2(1,-1) +iv2(0,9);
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

	std::vector<iv2> snake_body;

	for (;;) {
		snake_body.insert(snake_body.begin(), cur * iv2(1,-1) +iv2(0,9));

		auto body = get_snake_body(cur);
		if (body == 'O')
			break; // cur is head

		cur += get_snake_dir(body);
	}

	return snake_body;
}

auto snake = get_test_snake();

struct Snake_Renderer {
	
	Texture2D atlas = upload_texture_from_file("snake.png", { PF_SRGBA8, USE_MIPMAPS, FILTER_NEAREST, BORDER_CLAMP });
	iv2 atlas_size = iv2(4,4); // in sprites

	void draw_snake_body_piece (int i) {
	
		static iv2 atlas_head =				iv2(0,3);
		static iv2 atlas_tail =				iv2(2,0);
		static iv2 atlas_body_horiz[3] =	{ iv2(1,3), iv2(1,2), iv2(1,0) };
		static iv2 atlas_body_vert[3] =		{ iv2(0,1), iv2(1,1), iv2(2,1) };
		static iv2 atlas_body_curve[4] =	{ iv2(2,3), iv2(2,2), iv2(0,2), iv2(0,0) };

		iv2 body = snake[i];

		iv2 dir_towards_head;
		iv2 dir_from_tail;
		{
			iv2* towards_head = i > 0 ?							&snake[i -1] : nullptr;
			iv2* towards_tail = i < (int)snake.size() -1 ?		&snake[i +1] : nullptr;

			dir_towards_head = towards_head ? *towards_head -body : 0;
			dir_from_tail = towards_tail ? body -*towards_tail : 0;
		}

		iv2 tile = 0;
		iv2 tile_dir = iv2(-1,0);

		if (i == 0) { // head

			tile = atlas_head;
			tile_dir = dir_from_tail;

		} else if (i == ((int)snake.size() -1)) { // tail

			tile = atlas_tail;
			tile_dir = dir_towards_head;

		} else { 

			bool is_straight_body = all(dir_from_tail == dir_towards_head);

			if (is_straight_body) { // straight body piece

				bool is_horiz_body = dir_towards_head.x != 0;
				if (is_horiz_body)
					tile = atlas_body_horiz[i % 3];
				else
					tile = atlas_body_vert[i % 3];

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

		}

		int rot;
		if (		tile_dir.x == -1 )
			rot = 0;
		else if (	tile_dir.y == -1 )
			rot = 1;
		else if (	tile_dir.x == +1 )
			rot = 2;
		else /*if (tile_dir.y == +1 )*/
			rot = 3;

		draw_sprite((v2)body, v2(1), rot, tile, atlas_size, atlas);

	}

	void draw_apple (iv2 pos) {
		static iv2 atlas_apple =			iv2(3,3);

		draw_sprite((v2)pos, v2(1), 0, atlas_apple, atlas_size, atlas);
	}

	void draw_bg_tile (iv2 pos) {
		static iv2 atlas_bg_tile =			iv2(3,2);

		int random_value = std::hash<int>()(pos.x) * 13 + std::hash<int>()(pos.y) * 17;
		int random_rot = random_value % 4;

		draw_sprite((v2)pos, v2(1), random_rot, atlas_bg_tile, atlas_size, atlas);
	}
};

void frame (Display& dsp, Input& inp, flt dt) {

	static Camera2D cam = [] () {
		Camera2D cam;
		cam.key_reset = 'T';
		cam.fixed = true;
		cam.base_vsize_world = (flt)snake_cells.y;
		cam.pos_world = (v2)snake_cells / 2;
		return cam;
	} ();

	static int score = 0;

	{
		draw_to_screen(inp.wnd_size_px);
		clear(0);

		v2 snake_aspect = (v2)snake_cells;

		// black bars
		v2 tmp = (v2)inp.wnd_size_px / snake_aspect;
		tmp = min(tmp.x,tmp.y);

		iv2 frame_size_px = (iv2)(tmp * snake_aspect);

		iv2 frame_offs = (inp.wnd_size_px -frame_size_px) / 2;

		draw_to_screen(frame_offs, frame_size_px);

		cam.update(inp, dt, { frame_offs, frame_size_px });

		set_shared_uniform("view", "world_to_cam", cam.world_to_cam.m4());
		set_shared_uniform("view", "cam_to_clip", cam.cam_to_clip);
	}
	
	static Snake_Renderer renderer;

	for (int y=0; y<snake_cells.y; ++y) {
		for (int x=0; x<snake_cells.x; ++x) {
			renderer.draw_bg_tile(iv2(x,y));
		}
	}

	for (int i=0; i<(int)snake.size(); ++i) {
		renderer.draw_snake_body_piece(i);
	}

	renderer.draw_apple(apple_pos);
}

int main () {
	srand((uint)time(NULL));

	run_display(frame, MSVC_PROJECT_NAME);
	return 0;
}
