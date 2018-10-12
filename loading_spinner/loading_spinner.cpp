
#include "3d_lib/engine.hpp"
#include "3d_lib/camera2D.hpp"
using namespace engine;

struct Snake_Game : engine::Application {
	
	Camera2D cam = [] () {
		Camera2D c;
		c.size_world = 1.1f;
		return c;
	} ();

	void frame () {
		
		cam.update(inp, dt);
		cam.draw_to(inp,dt);

		clear(0);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);
		glDisable(GL_CULL_FACE);
		glDisable(GL_SCISSOR_TEST);

		inline_shader("loading_spinner.vert", R"_SHAD(
			$include "common.vert"

			in		vec2	pos_model;
			in		vec2	uv;

			out		vec2	vs_uv;

			uniform	mat4	view_world_to_cam;
			uniform	mat4	view_cam_to_clip;

			void vert () {
				gl_Position = view_cam_to_clip * view_world_to_cam * vec4(pos_model, 0,1);
				vs_uv = uv;
			}
		)_SHAD");
		inline_shader("loading_spinner.frag", R"_SHAD(
			$include "common.frag"
			
			in		vec2	vs_uv;

			uniform float anim_t = 0;
			uniform float min_r = 0.4;
			uniform float max_r = 0.6;
			uniform float fade_pow = 1.0f / 1.5;

			uniform vec2 pixel_size;

			vec2 unit_rot (float ang) {
				return vec2(cos(ang), sin(ang));
			}

			vec4 frag () {
				float rounded_ang = deg(20);
				
				vec2 pos = (vs_uv -0.5) * 2; // [-1,+1] range
				float r = length(pos);

				float ang = mod(atan(pos.y, pos.x), deg(360));
				float anim_ang = anim_t * deg(360);

				float fade = mod((anim_ang -ang) / deg(360), 1);
				fade = pow(fade, fade_pow);

				float ps = length(pixel_size) / 2;

				float alpha = 1;

				alpha *= clamp(map(r, min_r -ps, min_r +ps, 0,1), 0,1);
				alpha *= clamp(map(r, max_r -ps, max_r +ps, 1,0), 0,1);
				alpha *= 1 -fade;

				//vec2 rounded_circ_p = unit_rot(anim_ang -rounded_ang) * (min_r + max_r) / 2;
				//alpha *= 1 -max(length(pos -rounded_circ_p) / ((max_r -min_r) / 2), 0);

				return vec4(0.1, 1.0, 0.1,  alpha);
			}
		)_SHAD");

		static flt anim_period = 1;
		static flt anim_t = 0;
		static flt min_r = 0.4f;
		static flt max_r = 0.6f;
		static flt fade_pow = 1.0f / 1.4f;

		imgui::DragFloat("anim_period", &anim_period, 1.0f / 50);
		
		anim_t += (1.0f / anim_period) * dt;
		anim_t = mymod(anim_t, 1.0f);

		imgui::SliderFloat("anim_t", &anim_t, 0, 1);
		imgui::SliderFloat("min_r", &min_r, 0, 1);
		imgui::SliderFloat("max_r", &max_r, 0, 1);
		imgui::SliderFloat("fade_pow", &fade_pow, 0, 2);
		
		auto* s = use_shader("loading_spinner");
		if (s) {
			
			set_uniform(s, "min_r", min_r);
			set_uniform(s, "max_r", max_r);
			set_uniform(s, "anim_t", anim_t);
			set_uniform(s, "fade_pow", fade_pow);

			v2 size_px = (cam.world_to_cam.m4() * cam.cam_to_clip * v4(0.5f,0.5f, 0,0)).xy() / 2 * (v2)cam.get_subrect().size_px;

			set_uniform(s, "pixel_size", 1.0f / size_px);

			static auto rect = engine::gen_rect<Texture_Draw_Rect>([] (v2 p, v2 uv) { return Texture_Draw_Rect{p, uv}; }).upload();
			rect.draw(*s);
		}
	}

} app;

int main () {
	app.open(MSVC_PROJECT_NAME, iv2(200,200), VSYNC_ON);
	app.run();
	return 0;
}
