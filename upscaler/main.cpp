
#include "3d_lib/engine.hpp"
#include "3d_lib/camera2D.hpp"
#include "mylibs/find_files.hpp"
#include "3d_lib/common_colors.hpp"
using namespace n_find_files;
using namespace engine;
using namespace common_colors;

#define MEGA (1024*1024)

struct Texture_Cacher {
	
	struct Texture {
		Texture2D	tex;
		iv2			size_px;
		u64			size_bytes;
	};

	u64 max_size = 512ull *MEGA;
	u64 total_size = 0;

	std::unordered_map<std::string, unique_ptr<Texture>>	cached;

	void evict_any_but (std::string filepath) {
		std::vector<decltype(cached)::iterator> sorted;

		total_size = 0;

		for (auto it=cached.begin(); it!=cached.end(); ++it) {
			if (it->first != filepath) {
				sorted.push_back(it);
				total_size += it->second->size_bytes;
			}
		}

		std::sort(sorted.begin(), sorted.end(), [] (decltype(cached)::iterator l, decltype(cached)::iterator r) { return std::less<u64>()(l->second->size_bytes, r->second->size_bytes); });

		total_size = 0;

		for (auto it : sorted) {
			total_size += it->second->size_bytes;
			if (total_size > max_size) {
				cached.erase(it);
			}
		}
	}

	bool is_cached (std::string const& filepath) {
		auto it = cached.find(filepath);
		return it != cached.end();
	}

	Texture2D* get_texture (std::string const& filepath, iv2* size_px) {
		evict_any_but(filepath);

		auto it = cached.find(filepath);
		if (it != cached.end()) {
			*size_px = it->second->size_px;
			return &it->second->tex;
		}

		Texture tex;
		tex.tex = upload_texture_from_file(filepath.c_str(), { PF_SRGBA8, USE_MIPMAPS, FILTER_LINEAR, BORDER_CLAMP }, &tex.size_px, &tex.size_bytes);

		if (tex.tex.is_null())
			return nullptr;

		auto res = cached.emplace(filepath, make_unique<Texture>(std::move( std::move(tex) )));

		*size_px = res.first->second->size_px;
		return &res.first->second->tex;
	}

};

Texture_Cacher texture_cacher;

struct Show_Dir {
	Input& inp;
	std::string* selected_file;
	bool selected_file_changed;

	bool go_to_prev;
	bool go_to_next;

	void recurse (Directory_Tree const& dir, std::string const& path, int depth = 0) {
	
		bool def_open = depth == 0 || (depth == 1 && (dir.dirs.size() +dir.filenames.size()) < 20);
	
		bool open = imgui::TreeNodeEx(dir.name.c_str(), def_open ? ImGuiTreeNodeFlags_DefaultOpen : 0);

		for (auto& d : dir.dirs) {
			recurse(d, path+d.name, depth +1);
		}

		for (int i=0; i<(int)dir.filenames.size(); ++i) {
			auto& f = dir.filenames[i];

			auto filepath = path + f;

			bool cached = texture_cacher.is_cached(filepath);
			if (cached && open) imgui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f,1,0.8f,1));

			bool selected = filepath == *selected_file;
			bool selected_changed = open && imgui::Selectable(f.c_str(), &selected);

			if (selected && (go_to_prev || go_to_next)) {
				if (go_to_prev && i > 0) {
					*selected_file = path + dir.filenames[i -1];
					selected_file_changed = true;
				} else if (go_to_next && i < ((int)dir.filenames.size() -1)) {
					*selected_file = path + dir.filenames[i +1];
					selected_file_changed = true;
				}
				go_to_prev = go_to_next = false;
			} else {
				if (selected_changed) {
					*selected_file = selected ? filepath : "";
					selected_file_changed = true;
				}
			}

			if (cached && open) imgui::PopStyleColor();
		}

		if (open) imgui::TreePop();
	}

	static bool show (Input& inp, Directory_Tree const& dir, std::string* selected_file) {
		auto sd = Show_Dir{inp};
		sd.selected_file = selected_file;

		sd.selected_file_changed = false;

		sd.go_to_prev = inp.went_down_repeat(GLFW_KEY_LEFT);
		sd.go_to_next = inp.went_down_repeat(GLFW_KEY_RIGHT);

		sd.recurse(dir, dir.name);

		return sd.selected_file_changed;
	}
};

bool file_select (Input& inp, Texture2D** tex, iv2* size_px) {
	static bool init = true;

	static std::string folder = "images/";//"J:/upscaler_test";
	bool folder_changed = imgui::InputText_str("folder", &folder) || init;
	static bool folder_ok = false;
	imgui::SameLine();	imgui::TextColored(folder_ok ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1), folder_ok ? "OK" : "FAIL!");

	init = false;

	static unique_ptr<Directory_Watcher> dw;
	static Directory_Tree dir;

	folder_changed = folder_changed || (dw && dw->poll_file_changes());

	if (folder_changed) {
		find_files_recursive(folder, &dir);
		dw = make_unique<Directory_Watcher>(folder);

		folder_ok = dw->directory_is_valid();
	}

	if (!folder_ok)
		return false;

	static std::string selected_file = "images/test.png";//"J:/upscaler_test/01.jpg";
	bool selected_file_changed = Show_Dir::show(inp, dir, &selected_file);
	
	if (selected_file.size() == 0)
		*tex = nullptr;
	else
		*tex = texture_cacher.get_texture(selected_file, size_px);

	return selected_file_changed;
}

struct App : public Application {

	Camera2D cam;

	void frame () {

		static bool wireframe_enable = false;
		save->value("wireframe_enable", &wireframe_enable);
		imgui::Checkbox("wireframe_enable", &wireframe_enable);
		engine::set_shared_uniform("wireframe", "enable", wireframe_enable);

		static iv2 size_px;
		static Texture2D* tex;
		bool tex_changed = file_select(inp, &tex, &size_px);

		//
		engine::draw_to_screen(inp.wnd_size_px);
		engine::clear(npp_obsidian::bg_color);
	
		cam.update(inp, dt);
		cam.draw_to();

		if (tex) {
			flt tex_aspect = (flt)size_px.x / (flt)size_px.y;
		
			flt cam_aspect = (flt)inp.wnd_size_px.x / (flt)inp.wnd_size_px.y;

			v2 tex_size_world;
			if (cam_aspect > tex_aspect)
				tex_size_world = v2(tex_aspect, 1);
			else
				tex_size_world = v2(cam_aspect, cam_aspect / tex_aspect);

			v2 visible_size_px = cam.size_world * tex_size_world;
			v2 magnification = visible_size_px / (v2)size_px;
			//assert(equal_epsilon(magnification.x, magnification.y, 0.05f));

			/*
			magnification *= 100;
			bool set_magnif = imgui::DragFloat("image magnification", &magnification.y);
			magnification /= 100;

			if (set_magnif) {
				v2 visible_size_px = (v2)size_px * magnification.y;
				v2 cam_size_world = (v2)inp.wnd_size_px / (visible_size_px * tex_size_world);

				cam.size_world = cam_size_world;
			}*/

			static bool filter_nearest = false;
			if (imgui::Checkbox("filter_nearest", &filter_nearest) || tex_changed)
				tex->set_minmag_filtering(filter_nearest ? FILTER_NEAREST : FILTER_LINEAR, USE_MIPMAPS);
		
			static bool disable_filter = false;
			imgui::Checkbox("disable_filter", &disable_filter);


			static flt step_size_px = 1;
			imgui::DragFloat("step_size_px", &step_size_px, 0.1f / 40);

			static flt threshold = 0.2f;
			imgui::DragFloat("threshold", &threshold, 0.01f / 30);

			static int gradient_normal_samples = 8;
			imgui::DragInt("gradient_normal_samples", &gradient_normal_samples, 1.0f / 20);

			{
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glDisable(GL_DEPTH_TEST);
				glDepthMask(GL_TRUE);
				glDisable(GL_CULL_FACE);
				glDisable(GL_SCISSOR_TEST);

				inline_shader("texture_filter_common.vert", R"_SHAD(
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
				inline_shader("texture_filter_common.frag", R"_SHAD(
					$include "common.frag"
			
					in		vec2		vs_uv;

					uniform vec2		tex_size_px;
					uniform sampler2D	tex;

					vec4 filter ();

					vec4 frag () {
						return filter();
					}
				)_SHAD");

				inline_shader("texture_filter_no_filter.vert", R"_SHAD(
					$include "texture_filter_common.vert"
				)_SHAD");
				inline_shader("texture_filter_no_filter.frag", R"_SHAD(
					$include "texture_filter_common.frag"
				
					vec4 filter () { return texture(tex, vs_uv); }
				)_SHAD");

				inline_shader("texture_filter.vert", R"_SHAD(
					$include "texture_filter_common.vert"
				)_SHAD");

				auto* s = use_shader(disable_filter ? "texture_filter_no_filter" : "texture_filter");
				if (s) {

					hm model_to_world = scaleH(v3(tex_size_world,1));
					set_uniform(s, "model_to_world", model_to_world.m4());

					set_uniform(s, "tex_size_px", (v2)size_px);
					bind_texture(s, "tex", 0, *tex);


					set_uniform(s, "step_size_px", step_size_px);
					set_uniform(s, "threshold", threshold);
					set_uniform(s, "gradient_normal_samples", gradient_normal_samples);

					static auto rect = engine::gen_rect<Texture_Draw_Rect>([] (v2 p, v2 uv) { return Texture_Draw_Rect{p, uv}; }).upload();
					rect.draw(*s);
				}
			}
		}
	}
};

int main () {
	App app;
	app.open(MSVC_PROJECT_NAME);
	app.run();
	return 0;
}