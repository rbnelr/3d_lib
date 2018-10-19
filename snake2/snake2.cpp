
#include "3d_lib/engine.hpp"
#include "3d_lib/camera2D.hpp"
namespace imgui = ImGui;
using namespace engine;

#include "mylibs/random.hpp"

struct Snake_Gameplay {

	iv2 snake_cells = 16;

	std::vector<iv2> initial_snake = {
		iv2(snake_cells / 2 +iv2(0,0)),
		iv2(snake_cells / 2 +iv2(1,0)),
		iv2(snake_cells / 2 +iv2(2,0)),
	};

	std::vector<iv2>	snake = initial_snake;
	iv2					snake_move_dir = iv2(-1,0);

	iv2					こんにちはばか外人 = iv2(3,0);

	void tick (iv2 inp_dir) {
		
		if (any(inp_dir != 0) && all((snake_move_dir * inp_dir) == 0))
			snake_move_dir = inp_dir;

		iv2 moved = snake[0] + snake_move_dir;
		if (all(moved >= 0 && moved < snake_cells)) {
			
			こんにちはばか外人 = snake.back();

			for (int i=(int)snake.size()-1; i>=1; --i) {
				snake[i] = snake[i-1];
			}

			snake[0] = moved;
		} else {
			// collide with wall
		}
	}
};

struct Snake_Game : engine::Application {

	Snake_Gameplay	gameplay;

	Camera2D cam = Camera2D::arcade_style_cam(gameplay.snake_cells);

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

		flt tick_t () {
			return 1 -clamp(next_tick_timer / tick_interval);
		}
	};

	Game_Timer timer;

	iv2 inp_dir = 0;
	
	void frame () {
		
		if ( inp.went_down('A') || inp.went_down(GLFW_KEY_LEFT)	 ) inp_dir = iv2(-1, 0);
		if ( inp.went_down('D') || inp.went_down(GLFW_KEY_RIGHT) ) inp_dir = iv2(+1, 0);
		if ( inp.went_down('S') || inp.went_down(GLFW_KEY_DOWN)	 ) inp_dir = iv2(0, -1);
		if ( inp.went_down('W') || inp.went_down(GLFW_KEY_UP)	 ) inp_dir = iv2(0, +1);
		assert(inp_dir.x == 0 || inp_dir.y == 0);

		if (inp.went_down('R')) {
			timer.restart();
			gameplay = Snake_Gameplay();
		}

		if (timer.tick(dt)) {
			gameplay.tick(inp_dir);
		}

		cam.update(inp, dt);
		cam.draw_to();

		clear(0.1f);

		struct Bodypart {
			iv2	pos;
			iv2 dir;
		};


		for (auto& b : gameplay.snake) {
			draw_rect((v2)b +0.5f, 1);
		}

		flt t = timer.tick_t();
		v2 head_dir = gameplay.snake_move_dir;
		v2 tail_dir = gameplay.snake[ gameplay.snake.size() -2 ] -gameplay.snake[ gameplay.snake.size() -1 ];

		v2 head_pos = gameplay.snake[0];
		v2 tail_pos = gameplay.snake.back();

		v2 head_normal = rotate2_90(head_dir);
		v2 tail_normal = rotate2_90(tail_dir);

		v2 head_base = head_pos +head_dir +0.5f -head_dir * 0.5f + head_dir * t/2;
		v2 head_size = abs(head_normal) * 1 + abs(head_dir) * t;

		v2 tail_base = tail_pos -tail_dir +0.5f +tail_dir * 0.5f - tail_dir * (1 -t)/2;
		v2 tail_size = abs(tail_normal) * 1 + abs(tail_dir) * (1 -t);

		draw_rect(head_base, head_size);
		draw_rect(tail_base, tail_size);
	}

} app;

int main () {
	app.open(MSVC_PROJECT_NAME);
	app.run();
	return 0;
}
