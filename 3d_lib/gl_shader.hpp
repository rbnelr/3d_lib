#pragma once

#include "engine_include.hpp"
#include "dear_imgui.hpp"

#include "mylibs/parse.hpp"
#include "mylibs/directory_watcher.hpp"

#include <unordered_map>

#include "mylibs/containers.hpp"

#include "Imgui_Window_Button.hpp"

namespace engine {
using namespace simple_file_io;
//

class Shader {
	MOVE_ONLY_CLASS(Shader)

	GLuint prog_handle = 0;

public:
	~Shader () {
		if (prog_handle) // maybe this helps to optimize out destructing of unalloced shaders
			glDeleteProgram(prog_handle); // would be ok to delete unalloced shaders (handle = 0)
	}
	static Shader take_handle (GLuint h) {
		Shader shad;
		shad.prog_handle = h;
		return shad;
	}

	GLuint	get_prog_handle () const {	return prog_handle; }
};
inline void swap (Shader& l, Shader& r) {
	::std::swap(l.prog_handle, r.prog_handle);
}

#define INLINE_SHADER_RELOADING 1

struct Shader_Manager {
	
	std::vector<std::string> shader_source_folders = { "shaders/", "../3d_lib/shaders/" };

	void register_file_dependency (std::string const& filepath, std::vector<std::string>* file_dependencies) {
		if (file_dependencies && !contains(*file_dependencies, filepath))
			file_dependencies->push_back(filepath);
	}
	bool load_shader_source (std::string const& filename, std::string* source, std::vector<std::string>* file_dependencies) {
		{
			auto filepath = "<inline_shaders>/"+ filename;
			if (get_inline_shader_source(filepath, source)) {
				register_file_dependency(filepath, file_dependencies);
				return true;
			}
		}
		for (auto& sf : shader_source_folders) {
			auto filepath = sf+ filename;
			if (load_text_file(filepath.c_str(), source)) {
				register_file_dependency(filepath, file_dependencies);
				return true;
			}
		}
		return false;
	}

	bool recursive_preprocess_shader (std::string const& filename, std::string* source, std::vector<std::string>* included_files, std::vector<std::string>* file_dependencies=nullptr) {
		included_files->push_back(filename);

		auto find_path = [] (std::string const& filepath) { // "shaders/blah.vert" -> "shaders/"  "shaders" -> ""  "blah\\" -> "\\"
			auto filename_pos = filepath.find_last_of("/\\");
			return filepath.substr(0, filename_pos +1);
		};

		std::string path = find_path(filename);

		if (!load_shader_source(filename, source, file_dependencies)) {
			errprint("Could load shader source! \"%s\"\n", filename.c_str());
			return false;
		}

		using namespace n_parse;

		int		line_number = 0; // in original file, this ignores all replaced lines

		int		cur_line_begin_indx = 0;
		char*	cur = (char*)&source->c_str()[cur_line_begin_indx];

		auto go_to_next_line = [&] () {
			while (!end_of_line(&cur))
				++cur;
		};

		auto replace_line_with = [&] (std::string lines) { // line should have been completed with end_of_line()
			auto cur_line_end_indx = cur -(char*)source->c_str();

			source->replace(	cur_line_begin_indx, cur -&source->c_str()[cur_line_begin_indx],
							lines);
			cur_line_end_indx += lines.size() -(cur_line_end_indx -cur_line_begin_indx);

			cur = (char*)&source->c_str()[cur_line_end_indx]; // source->replace invalidates cur
		};
		auto comment_out_cur_line = [&] () { // line should have been completed with end_of_line()
			auto cur_line_end_indx = cur -(char*)source->c_str();

			source->insert(cur_line_begin_indx, "//");
			cur_line_end_indx += 2;

			cur = (char*)&source->c_str()[cur_line_end_indx]; // source->replace invalidates cur
		};

		auto include_file = [&] (std::string include_filepath) -> bool { // line should have been completed with end_of_line()

			include_filepath.insert(0, path);

			if (contains(*included_files, include_filepath)) {
				// file already include, we prevent double include by default
				replace_line_with("//include \"%s\" (prevented double-include)\n");
			} else {
				std::string included_source;
				if (!recursive_preprocess_shader(include_filepath, &included_source, included_files, file_dependencies)) return false;

				replace_line_with(prints(	"//$include \"%s\"\n"
										 "%s\n"
										 "//$include_end file \"%s\" line %d\n",
										 include_filepath.c_str(), included_source.c_str(), filename.c_str(), line_number));
			}

			return true;
		};

		auto dollar_cmd = [&] (char** pcur) {
			char* cur = *pcur;

			whitespace(&cur);

			if (!character(&cur, '$')) return false;

			whitespace(&cur);

			*pcur = cur;
			return true;
		};

		auto include_cmd = [&] () {
			if (!identifier(&cur, "include")) return false;

			whitespace(&cur);

			std::string include_filepath;
			if (!quoted_string_copy(&cur, &include_filepath)) return false;

			if (!end_of_line(&cur)) return false;

			if (!include_file(std::move(include_filepath))) return false;

			return true;
		};

		while (!end_of_input(cur)) { // for all lines

			if ( dollar_cmd(&cur) ) {
				if (		include_cmd() );
				//else if (	other command );
				else {
					errprint("unknown or invalid $command in shader \"%s\".\n", filename.c_str());

					// ignore invalid line
					go_to_next_line();
					comment_out_cur_line();
				}
			} else {
				go_to_next_line();
			}

			cur_line_begin_indx = (int)(cur -(char*)source->c_str()); // set beginning of next line
			++line_number;
		}

		return true;
	}
	bool preprocess_shader (std::string const& filename, std::string* source, std::vector<std::string>* file_dependencies=nullptr) {
		std::vector<std::string> included_files; // included_files in this shader (file_dependencies are for entire shader program, so i can't use the same list of files here)
		return recursive_preprocess_shader(filename, source, &included_files, file_dependencies);
	}

	bool get_shader_compile_log (GLuint shad, std::string* log) {
		GLsizei log_len;
		{
			GLint temp = 0;
			glGetShaderiv(shad, GL_INFO_LOG_LENGTH, &temp);
			log_len = (GLsizei)temp;
		}

		if (log_len <= 1) {
			return false; // no log available
		} else {
			// GL_INFO_LOG_LENGTH includes the null terminator, but it is not allowed to write the null terminator in str, so we have to allocate one additional char and then resize it away at the end

			log->resize(log_len);

			GLsizei written_len = 0;
			glGetShaderInfoLog(shad, log_len, &written_len, &(*log)[0]);
			assert(written_len == (log_len -1));

			log->resize(written_len);

			return true;
		}
	}
	bool get_program_link_log (GLuint prog, std::string* log) {
		GLsizei log_len;
		{
			GLint temp = 0;
			glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &temp);
			log_len = (GLsizei)temp;
		}

		if (log_len <= 1) {
			return false; // no log available
		} else {
			// GL_INFO_LOG_LENGTH includes the null terminator, but it is not allowed to write the null terminator in str, so we have to allocate one additional char and then resize it away at the end

			log->resize(log_len);

			GLsizei written_len = 0;
			glGetProgramInfoLog(prog, log_len, &written_len, &(*log)[0]);
			assert(written_len == (log_len -1));

			log->resize(written_len);

			return true;
		}
	}

	bool load_gl_shader (GLenum type, std::string const& filename, GLuint* shad, std::string* source, std::vector<std::string>* file_dependencies=nullptr) {
		*shad = glCreateShader(type);

		if (!preprocess_shader(filename, source, file_dependencies)) return false;

		{
			cstr ptr = source->c_str();
			glShaderSource(*shad, 1, &ptr, NULL);
		}

		glCompileShader(*shad);

		bool success;
		{
			GLint status;
			glGetShaderiv(*shad, GL_COMPILE_STATUS, &status);

			std::string log_str;
			bool log_avail = get_shader_compile_log(*shad, &log_str);

			success = status == GL_TRUE;
			if (!success) {
				// compilation failed
				errprint("OpenGL error in shader compilation \"%s\"!\n>>>\n%s\n<<<\n", filename.c_str(), log_avail ? log_str.c_str() : "<no log available>");
			} else {
				// compilation success
				if (log_avail) {
					errprint("OpenGL shader compilation log \"%s\":\n>>>\n%s\n<<<\n", filename.c_str(), log_str.c_str());
				}
			}
		}

		return success;
	}
	GLuint load_gl_shader_program (std::string const& vert_filename, std::string const& frag_filename, std::string* vert_src, std::string* frag_src, std::vector<std::string>* file_dependencies=nullptr) {
		GLuint prog_handle = glCreateProgram();

		GLuint vert;
		GLuint frag;

		bool compile_success = true;

		bool vert_success = load_gl_shader(GL_VERTEX_SHADER,		vert_filename, &vert, vert_src, file_dependencies);
		bool frag_success = load_gl_shader(GL_FRAGMENT_SHADER,		frag_filename, &frag, frag_src, file_dependencies);

		if (!(vert_success && frag_success)) {
			glDeleteProgram(prog_handle);
			prog_handle = 0;
			return 0;
		}

		glAttachShader(prog_handle, vert);
		glAttachShader(prog_handle, frag);

		glLinkProgram(prog_handle);

		bool success;
		{
			GLint status;
			glGetProgramiv(prog_handle, GL_LINK_STATUS, &status);

			std::string log_str;
			bool log_avail = get_program_link_log(prog_handle, &log_str);

			success = status == GL_TRUE;
			if (!success) {
				// linking failed
				errprint("OpenGL error in shader linkage \"%s\"|\"%s\"!\n>>>\n%s\n<<<\n", vert_filename.c_str(), frag_filename.c_str(), log_avail ? log_str.c_str() : "<no log available>");
			} else {
				// linking success
				if (log_avail) {
					errprint("OpenGL shader linkage log \"%s\"|\"%s\":\n>>>\n%s\n<<<\n", vert_filename.c_str(), frag_filename.c_str(), log_str.c_str());
				}
			}
		}

		glDetachShader(prog_handle, vert);
		glDetachShader(prog_handle, frag);

		glDeleteShader(vert);
		glDeleteShader(frag);

		return prog_handle;
	}

	struct Cached_Shader {
		Shader	shad;

		std::vector<std::string> file_dependencies; // if any file this shader is made out of is changed reload the shader

		std::string vert_src;
		std::string frag_src;
	};

	Cached_Shader load_shader (std::string const& name) {
		Cached_Shader cs;

		auto h = load_gl_shader_program(	name +".vert",	name +".frag",
											&cs.vert_src,	&cs.frag_src,
											&cs.file_dependencies );

		cs.shad = Shader::take_handle(h);
		
		return cs;
	}

	//
	std::unordered_map<std::string, Cached_Shader> shaders;

	Shader* get_shader (std::string const& name) {
		auto shad = shaders.find(name);
		if (shad == shaders.end()) {
			Cached_Shader s;

			s = load_shader(name);
			if (s.shad.get_prog_handle() == 0)
				return nullptr;

			shad = shaders.emplace(name, std::move(s)).first;
		}
		return &shad->second.shad;
	}

	struct Inline_Shader_File {
		std::string	source;

		#if INLINE_SHADER_RELOADING
		bool		was_changed = false;
		#endif

		Inline_Shader_File (std::string const& source): source{source} {}
	};

	std::unordered_map<std::string, Inline_Shader_File> inline_shader_files;

	bool get_inline_shader_source (std::string const& virtual_filepath, std::string* source) {
		auto is = inline_shader_files.find(virtual_filepath);
		if (is == inline_shader_files.end())
			return false;

		*source = is->second.source;
		return true;
	}
	void inline_shader (std::string virtual_filepath, std::string const& source) {
		virtual_filepath.insert(0, "<inline_shaders>/");

		auto is = inline_shader_files.find(virtual_filepath);
		if (is == inline_shader_files.end()) {
			
			is = inline_shader_files.emplace(virtual_filepath, source).first;

		} else {
			
			#if INLINE_SHADER_RELOADING
			is->second.was_changed = is->second.source.compare(source) != 0;
			if (is->second.was_changed)
				is->second.source = source;
			#endif
		}
	}

	std::vector<unique_ptr<Directory_Watcher>> dir_watchers;
	
	struct Shader_Window {
		static constexpr cstr window_name = "Shader_Window";

		std::string		shader = "";
		bool			open;

		void imgui (Shader_Manager* sm) {
			
			Cached_Shader* cs;

			auto res = sm->shaders.find(shader);
			cs = res != sm->shaders.end() ? &res->second : nullptr;
			
			if (ImGui::BeginCombo("shader", cs ? shader.c_str() : "<none>")) {

				for (auto& it : sm->shaders) {
					auto& name = it.first;
					Cached_Shader& s = it.second;

					bool is_selected = &s == cs;
					if (ImGui::Selectable(name.c_str(), is_selected))
						shader = name;
					if (is_selected)
						ImGui::SetItemDefaultFocus();
				}

				ImGui::EndCombo();
			}

			if (cs) {
				if (ImGui::CollapsingHeader("Vertex Shader Source", ImGuiTreeNodeFlags_DefaultOpen)) {
					auto& s = cs->vert_src;
					ImGui::TextUnformatted(&s[0], &s[ s.size() ]);
				}
				if (ImGui::CollapsingHeader("Fragment Shader Source", ImGuiTreeNodeFlags_DefaultOpen)) {
					auto& s = cs->frag_src;
					ImGui::TextUnformatted(&s[0], &s[ s.size() ]);
				}
			}
		}
	};

	Window_Button<Shader_Window> shader_windows;

	void poll_reload_shaders (int frame_i) {
		
		if (dir_watchers.size() == 0) {
			for (auto& sf : shader_source_folders)
				dir_watchers.emplace_back( make_unique<Directory_Watcher>(sf.c_str()) );
		}

		std::vector<std::string> changed_files;
		for (auto& dw : dir_watchers)
			dw->poll_file_changes_ignore_removed(&changed_files);

		for (auto& is : inline_shader_files) {
			if (is.second.was_changed) {
				changed_files.push_back( is.first );
				is.second.was_changed = false;
			}
		}

		for (auto& filepath : changed_files) {
			printf("frame %6d: \"%s\" changed\n", frame_i, filepath.c_str());
		}

		for (auto& shad : shaders) {
			auto& filepath = shad.first;

			if (any_contains(shad.second.file_dependencies, changed_files)) {
				// dependency changed

				Cached_Shader s = load_shader(filepath);

				if (s.shad.get_prog_handle() == 0) {
					// new shader could not be loaded, keep the old shader
					printf("  shader \"%s\" could not be loaded, keeping the old shader!\n", filepath.c_str());
				} else {
					printf("  reloading shader \"%s\".\n", filepath.c_str());
					shad.second = std::move(s); // overwrite old shader with new
				}
			}

		}

		shader_windows.imgui(this);
	}
};

Shader_Manager shader_manager;

void inline_shader (std::string const& virtual_filepath, std::string const& source) {
	shader_manager.inline_shader(virtual_filepath, source);
}

Shader const* _current_used_shader = nullptr; // for debugging

void gl_set_uniform (GLint loc, flt val) {		glUniform1f(	loc, val); }
void gl_set_uniform (GLint loc, fv2 val) {		glUniform2fv(	loc, 1, &val.x); }
void gl_set_uniform (GLint loc, fv3 val) {		glUniform3fv(	loc, 1, &val.x); }
void gl_set_uniform (GLint loc, fv4 val) {		glUniform4fv(	loc, 1, &val.x); }
void gl_set_uniform (GLint loc, s32 val) {		glUniform1i(	loc, val); }
void gl_set_uniform (GLint loc, s32v2 val) {	glUniform2iv(	loc, 1, &val.x); }
void gl_set_uniform (GLint loc, s32v3 val) {	glUniform3iv(	loc, 1, &val.x); }
void gl_set_uniform (GLint loc, s32v4 val) {	glUniform4iv(	loc, 1, &val.x); }
void gl_set_uniform (GLint loc, fm2 val) {		glUniformMatrix2fv(	loc, 1, GL_FALSE, &val.arr[0].x); }
void gl_set_uniform (GLint loc, fm3 val) {		glUniformMatrix3fv(	loc, 1, GL_FALSE, &val.arr[0].x); }
void gl_set_uniform (GLint loc, fm4 val) {		glUniformMatrix4fv(	loc, 1, GL_FALSE, &val.arr[0].x); }
void gl_set_uniform (GLint loc, bool val) {		glUniform1i(	loc, (int)val); }
void gl_set_uniform (GLint loc, bv2 val) {		glUniform2i(	loc, (int)val.x,(int)val.y); }
void gl_set_uniform (GLint loc, bv3 val) {		glUniform3i(	loc, (int)val.x,(int)val.y,(int)val.z); }
void gl_set_uniform (GLint loc, bv4 val) {		glUniform4i(	loc, (int)val.x,(int)val.y,(int)val.z,(int)val.w); }


template <typename T> void set_uniform (Shader* shad, std::string const& name, T val) {
	assert(_current_used_shader == shad);

	GLint loc = glGetUniformLocation(shad->get_prog_handle(), name.c_str());
	if (loc >= 0) gl_set_uniform(loc, val);
}

void use_shader (Shader* shad) {
	_current_used_shader = shad;
	glUseProgram(shad->get_prog_handle());
}

struct Uniform_Sharer {
	enum type_e {
		FLT, FV2, FV3, FV4,
		INT_, IV2, IV3, IV4,

		U8V4,

		MAT2, MAT3, MAT4,

		BOOL, BV2, BV3, BV4,
	};
	struct Uniform_Val {
		type_e	type;
		union {
			f32		flt_;
			fv2		fv2_;
			fv3		fv3_;
			fv4		fv4_;

			s32		int_;
			s32v2	iv2_;
			s32v3	iv3_;
			s32v4	iv4_;

			fm2		fm2_;
			fm3		fm3_;
			fm4		fm4_;

			bool	bool_;
			bv2		bv2_;
			bv3		bv3_;
			bv4		bv4_;
		};

		Uniform_Val () {}

		void set (f32	val) { type = FLT ;	flt_  = val;	}
		void set (fv2	val) { type = FV2 ;	fv2_  = val;	}
		void set (fv3	val) { type = FV3 ;	fv3_  = val;	}
		void set (fv4	val) { type = FV4 ;	fv4_  = val;	}
		void set (s32	val) { type = INT_;	int_  = val;	}
		void set (s32v2	val) { type = IV2 ;	iv2_  = val;	}
		void set (s32v3	val) { type = IV3 ;	iv3_  = val;	}
		void set (s32v4	val) { type = IV4 ;	iv4_  = val;	}
		void set (m2	val) { type = MAT2;	fm2_  = val;	}
		void set (m3	val) { type = MAT3;	fm3_  = val;	}
		void set (m4	val) { type = MAT4;	fm4_  = val;	}
		void set (bool	val) { type = BOOL;	bool_ = val;	}
		void set (bv2	val) { type = BV2;	bv2_  = val;	}
		void set (bv3	val) { type = BV3;	bv3_  = val;	}
		void set (bv4	val) { type = BV4;	bv4_  = val;	}
	};
	static void gl_set_uniform (GLint loc, Uniform_Val const& val) {
		switch (val.type) {
			case FLT :	engine::gl_set_uniform(loc, val.flt_ );	break;
			case FV2 :	engine::gl_set_uniform(loc, val.fv2_ );	break;
			case FV3 :	engine::gl_set_uniform(loc, val.fv3_ );	break;
			case FV4 :	engine::gl_set_uniform(loc, val.fv4_ );	break;
			case INT_:	engine::gl_set_uniform(loc, val.int_ );	break;
			case IV2 :	engine::gl_set_uniform(loc, val.iv2_ );	break;
			case IV3 :	engine::gl_set_uniform(loc, val.iv3_ );	break;
			case IV4 :	engine::gl_set_uniform(loc, val.iv4_ );	break;
			case MAT2:	engine::gl_set_uniform(loc, val.fm2_ );	break;
			case MAT3:	engine::gl_set_uniform(loc, val.fm3_ );	break;
			case MAT4:	engine::gl_set_uniform(loc, val.fm4_ );	break;
			case BOOL:	engine::gl_set_uniform(loc, val.bool_);	break;
			case BV2:	engine::gl_set_uniform(loc, val.bv2_ );	break;
			case BV3:	engine::gl_set_uniform(loc, val.bv3_ );	break;
			case BV4:	engine::gl_set_uniform(loc, val.bv4_ );	break;
			default: assert(not_implemented);
		}
	}

	std::unordered_map<std::string, Uniform_Val>	shared_uniforms;

	void set_shared_uniform (std::string const& share_name, std::string const& uniform_name, Uniform_Val val) {
		shared_uniforms[share_name+ '_' +uniform_name] = val;
	}

	void set_shared_uniforms_for_shader (Shader* shad) {
		assert(_current_used_shader == shad);

		for (auto& su : shared_uniforms) {
			GLint loc = glGetUniformLocation(shad->get_prog_handle(), su.first.c_str());
			if (loc != -1) gl_set_uniform(loc, su.second);
		}
	}
};
Uniform_Sharer uniform_sharer;

/*
Set uniform that all shaders can access
This uniform even applies if the shader does not exist yet (i do lazy shader loading)

Could be optimized to use Uniform Buffer Object (share_name would be the name of a UBO and uniform_name would be a member of that structure)
*/
template <typename T> void set_shared_uniform (std::string const& share_name, std::string const& uniform_name, T val) {
	Uniform_Sharer::Uniform_Val unionized;
	unionized.set(val);
	uniform_sharer.set_shared_uniform(share_name, uniform_name, unionized);
}

// gl abstraction utility functions
Shader* _get_shader (std::string const& shader) {
	Shader*	shad = shader_manager.get_shader(shader);
	return shad;
}
Shader* use_shader (std::string const& shader) {
	Shader*	shad = _get_shader(shader);
	if (shad) {
		use_shader(shad);
		uniform_sharer.set_shared_uniforms_for_shader(shad);
	}
	return shad;
}

//
}
