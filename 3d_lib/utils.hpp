#pragma once

#include "engine_include.hpp"
#include "gl_shader.hpp"
#include "gl_mesh.hpp"
#include "gl_texture.hpp"

namespace engine {
//

	void clear (lrgb col) {
		glClearColor(col.x,col.y,col.z, 1);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	}

	m4 calc_perspective_matrix (flt vfov, flt clip_near, flt clip_far, flt aspect_w_over_h) {

		v2 frustrum_scale = tan(vfov * 0.5f);
		frustrum_scale.x *= aspect_w_over_h;

		v2 frustrum_scale_inv = 1 / frustrum_scale;

		f32 temp = clip_near -clip_far;

		f32 x = frustrum_scale_inv.x;
		f32 y = frustrum_scale_inv.y;
		f32 a = (clip_far +clip_near) / temp;
		f32 b = (2 * clip_far * clip_near) / temp;

		return m4::rows(	x, 0, 0, 0,
							0, y, 0, 0,
							0, 0, a, b,
							0, 0,-1, 0 );
	}

	// windows.h defines far & near
	#undef far
	#undef near

	m4 calc_orthographic_matrix (v2 size, flt near=-1, flt far=100) {
		flt x = 1.0f / (size.x / 2);
		flt y = 1.0f / (size.y / 2);

		flt a = 1.0f / (far -near);
		flt b = near * a;

		return m4::rows(	x, 0, 0, 0,
							0, y, 0, 0,
							0, 0, a, b,
							0, 0, 0, 1 );
	}

	struct Vertex_Draw_Rect {
		v2		pos_model;

		static const Vertex_Layout layout;
	};
	const Vertex_Layout Vertex_Draw_Rect::layout = { (int)sizeof(Vertex_Draw_Rect), {
		{ "pos_model",			FV2,	(int)offsetof(Vertex_Draw_Rect, pos_model) },
	}};

	void draw_rect (v2 pos, v2 size, lrgba col=1) {
		
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);
		glDisable(GL_CULL_FACE);
		glDisable(GL_SCISSOR_TEST);

		inline_shader("_simple_draw_rect.vert", R"_SHAD(
			$include "common.vert"

			in		vec2	pos_model;

			uniform	mat4	model_to_world;
			uniform	mat4	view_world_to_cam;
			uniform	mat4	view_cam_to_clip;

			void vert () {
				gl_Position = view_cam_to_clip * view_world_to_cam * model_to_world * vec4(pos_model, 0,1);
			}
		)_SHAD");
		inline_shader("_simple_draw_rect.frag", R"_SHAD(
			$include "common.frag"

			uniform vec4	col;

			vec4 frag () {
				return col;
			}
		)_SHAD");

		auto* s = use_shader("_simple_draw_rect");
		if (s) {

			hm model_to_world = translateH(v3(pos,1)) * scaleH(v3(size,1));

			set_uniform(s, "model_to_world", model_to_world.m4());
			set_uniform(s, "col", col);

			static auto rect = engine::gen_rect<Vertex_Draw_Rect>([] (v2 p, v2 uv) { return Vertex_Draw_Rect{p}; }).upload();
			rect.draw(*s);
		}
	}

	struct Texture_Draw_Rect {
		v2		pos_model;
		v2		uv;

		static const Vertex_Layout layout;
	};
	const Vertex_Layout Texture_Draw_Rect::layout = { (int)sizeof(Texture_Draw_Rect), {
		{ "pos_model",			FV2,	(int)offsetof(Texture_Draw_Rect, pos_model) },
		{ "uv",					FV2,	(int)offsetof(Texture_Draw_Rect, uv) },
	}};

	void draw_rect (v2 pos, v2 size, flt rot, v2 uv_pos, v2 uv_size, Texture2D const& tex, lrgba tint=1) {
		
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);
		glDisable(GL_CULL_FACE);
		glDisable(GL_SCISSOR_TEST);

		inline_shader("_simple_draw_rect.vert", R"_SHAD(
			$include "common.vert"

			in		vec2	pos_model;
			in		vec2	uv;

			out		vec2	vs_uv;

			uniform	mat4	model_to_world;
			uniform	mat4	view_world_to_cam;
			uniform	mat4	view_cam_to_clip;

			void vert () {
				gl_Position = view_cam_to_clip * view_world_to_cam * model_to_world * vec4(pos_model, 0,1);
				vs_uv = uv;
			}
		)_SHAD");
		inline_shader("_simple_draw_rect.frag", R"_SHAD(
			$include "common.frag"
			
			in		vec2	vs_uv;
			
			uniform vec2	uv_remap_l = vec2(0.0);
			uniform vec2	uv_remap_h = vec2(1.0);
			uniform vec4	tint;

			uniform sampler2D	tex;

			vec4 frag () {
				return texture(tex, mix(uv_remap_l, uv_remap_h, vs_uv)) * tint;
			}
		)_SHAD");

		auto* s = use_shader("_simple_draw_rect");
		if (s) {

			hm model_to_world = translateH(v3(pos,1)) * hm( rotate2(rot) * scale2(size) );

			set_uniform(s, "model_to_world", model_to_world.m4());
			set_uniform(s, "uv_remap_l", uv_pos);
			set_uniform(s, "uv_remap_h", uv_pos +uv_size);
			set_uniform(s, "tint", tint);

			bind_texture(s, "tex", 0, tex);

			static auto rect = engine::gen_rect<Texture_Draw_Rect>([] (v2 p, v2 uv) { return Texture_Draw_Rect{p, uv}; }).upload();
			rect.draw(*s);
		}
	}

	void draw_rect (v2 pos, v2 size, flt rot, Texture2D const& tex, lrgba tint=1) {
		draw_rect(pos, size, rot, 0,1, tex, tint);
	}
	void draw_rect (v2 pos, v2 size, Texture2D const& tex, lrgba tint=1) {
		draw_rect(pos, size, 0, tex, tint);
	}

	void draw_sprite (v2 pos, v2 size, int rot, iv2 tile_pos, iv2 tiles_count, Texture2D const& atlas, lrgba tint=1) {
		// When using linear filtering we get tile bleeding on every tile edge, since the uvs start and end exactly between two texels -> simply offsetting the pixels is actually wrong
		//  since then the border pixels are half as wide as they are supposed to be (?)
		//  only proper solution is 1 (or a few) pixels border around each tile (?)

		flt pixel_leak_fix = 0.001f; // only works for nearest filtering

		v2 uv_pos = (v2)tile_pos / (v2)tiles_count + pixel_leak_fix;
		v2 uv_size = 1 / (v2)tiles_count - pixel_leak_fix*2;

		draw_rect(pos +0.5f, size, deg(90) * rot, uv_pos, uv_size, atlas, tint);
	}

	struct Vertex_Draw_Lines {
		v2		pos_model;

		static const Vertex_Layout layout;
	};
	const Vertex_Layout Vertex_Draw_Lines::layout = { (int)sizeof(Vertex_Draw_Lines), {
		{ "pos_model",			FV2,	(int)offsetof(Vertex_Draw_Lines, pos_model) },
	}};

	void draw_lines (Cpu_Mesh<Vertex_Draw_Lines> lines, v2 pos, v2 size, lrgba col=1) {

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);
		glDisable(GL_CULL_FACE);
		glDisable(GL_SCISSOR_TEST);

		inline_shader("_simple_draw_rect.vert", R"_SHAD(
			$include "common.vert"

			in		vec2	pos_model;

			uniform	mat4	model_to_world;
			uniform	mat4	view_world_to_cam;
			uniform	mat4	view_cam_to_clip;

			void vert () {
				gl_Position = view_cam_to_clip * view_world_to_cam * model_to_world * vec4(pos_model, 0,1);
			}
		)_SHAD");
		inline_shader("_simple_draw_rect.frag", R"_SHAD(
			$include "common.frag"

			uniform vec4	col;

			vec4 frag () {
				return col;
			}
		)_SHAD");

		auto* s = use_shader("_simple_draw_rect");
		if (s) {

			hm model_to_world = translateH(v3(pos,1)) * scaleH(v3(size,1));

			set_uniform(s, "model_to_world", model_to_world.m4());
			set_uniform(s, "col", col);

			lines.upload().draw(LINES, *s);
		}
	}

	void draw_simple (Gpu_Mesh const& mesh, v3 pos_world, v3 scale=1, lrgba col=1) {

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);
		glEnable(GL_CULL_FACE);
		glDisable(GL_SCISSOR_TEST);

		inline_shader("_simple_draw_3d.vert", R"_SHAD(
			$include "common.vert"

			in		vec3	pos_model;
			in		vec3	normal_model;
			in		vec2	uv;

			out		vec3	vs_normal_world;
			out		vec2	vs_uv;

			uniform	mat4	model_to_world;
			uniform	mat4	view_world_to_cam;
			uniform	mat4	view_cam_to_clip;

			void vert () {
				gl_Position = view_cam_to_clip *	view_world_to_cam * model_to_world * vec4(pos_model,1);
				vs_normal_world = mat3(model_to_world) * normal_model;
				vs_uv = uv;
			}
		)_SHAD");
		inline_shader("_simple_draw_3d.frag", R"_SHAD(
			$include "common.frag"
			
			in		vec3	vs_normal_world;
			in		vec2	vs_uv;

			uniform vec4	albedo;
			uniform vec3	light_dir = normalize(vec3(-1, -2, +2.4));

			vec4 frag () {
				vec3 col = albedo.rgb;
				float alpha = albedo.a;

				col *= max(dot(normalize(vs_normal_world), light_dir), 0.0) +0.05;

				return vec4(col, alpha);
			}
		)_SHAD");

		auto* s = use_shader("_simple_draw_3d");
		if (s) {

			hm model_to_world = translateH(pos_world) * scaleH(scale);

			set_uniform(s, "model_to_world", model_to_world.m4());
			set_uniform(s, "albedo", col);

			mesh.draw(*s);
		}
	}

	//void draw_cube (v3 pos_world, v3 scale, lrgba col) {
	//	
	//	auto upload_cube = [] () {
	//		std::vector<Vertex> verts;
	//
	//		gen_cube(1, [&] (v3 p) { verts.push_back({p}); });
	//
	//		return VBO::gen_and_upload(verts.data(), (int)(verts.size() * sizeof(Vertex)));
	//	};
	//
	//	static auto vbo = upload_cube();
	//
	//	glEnable(GL_BLEND);
	//	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//	glEnable(GL_DEPTH_TEST);
	//	glEnable(GL_CULL_FACE);
	//	glDisable(GL_SCISSOR_TEST);
	//
	//	inline_shader("_simple_draw_3d.vert", R"_SHAD(
	//		$include "common.vert"
	//
	//		in		vec3	pos_model;
	//
	//		uniform	mat4	model_to_world;
	//		uniform	mat4	view_world_to_cam;
	//		uniform	mat4	view_cam_to_clip;
	//
	//		void vert () {
	//			gl_Position = view_cam_to_clip * view_world_to_cam * model_to_world * vec4(pos_model,1);
	//		}
	//	)_SHAD");
	//	inline_shader("_simple_draw_3d.frag", R"_SHAD(
	//		$include "common.frag"
	//
	//		uniform vec4	col;
	//
	//		vec4 frag () {
	//			return col;
	//		}
	//	)_SHAD");
	//
	//	auto* s = use_shader("_simple_draw_3d");
	//	if (s) {
	//
	//		use_vertex_data(*s, layout, vbo);
	//
	//		hm model_to_world = translateH(pos_world) * scaleH(radius);
	//
	//		set_uniform(s, "model_to_world", model_to_world.m4());
	//		set_uniform(s, "col", col);
	//
	//		draw_triangles(*s, 0, 6*6);
	//	}
	//}

	struct Vertex_Just_Pos {
		v3		pos_world;

		static const Vertex_Layout layout;
	};
	const Vertex_Layout Vertex_Just_Pos::layout = { (int)sizeof(Vertex_Just_Pos), {
		{ "pos_world",			FV3,	(int)offsetof(Vertex_Just_Pos, pos_world) },
	}};

	void draw_skybox_gradient () {

		glDisable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);
		glDisable(GL_CULL_FACE);
		glDisable(GL_SCISSOR_TEST);

		inline_shader("_draw_skybox_gradient.vert", R"_SHAD(
			$include "common.vert"

			in		vec3	pos_world;

			out		vec3	vs_pos_world_dir;

			uniform	mat4	model_to_world;
			uniform	mat4	view_world_to_cam;
			uniform	mat4	view_cam_to_clip;

			void vert () {
				vs_pos_world_dir = pos_world;
				gl_Position = view_cam_to_clip * vec4(mat3(view_world_to_cam) * vs_pos_world_dir, 1);
			}
		)_SHAD");
		inline_shader("_draw_skybox_gradient.frag", R"_SHAD(
			$include "common.frag"

			in		vec3	vs_pos_world_dir;

			vec4 frag () {
				vec3 dir = normalize(vs_pos_world_dir);

				vec3 sky_col =		srgb(190,239,255);
				vec3 horiz_col =	srgb(204,227,235);
				vec3 down_col =		srgb(41,49,52);

				vec3 col;
				if (dir.z > 0)
					col = mix(horiz_col, sky_col, dir.z);
				else
					col = mix(horiz_col, down_col, -dir.z);

				return vec4(col,1);
			}
		)_SHAD");

		auto* s = use_shader("_draw_skybox_gradient");
		if (s) {

			static auto cube = engine::gen_cube<Vertex_Just_Pos>([] (v3 p, v3 n, v2 u, int f) { return Vertex_Just_Pos{p}; }).upload();
			cube.draw(*s);
		}

	}
//
}
