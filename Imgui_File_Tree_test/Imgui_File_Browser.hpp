#pragma once

#include "3d_lib/engine.hpp"
#include "deps/mylibs/find_files.hpp"

namespace imgui_file_browser {
using namespace engine;

struct Dynamic_File_Tree {

	struct File_Or_Dir {
		bool		is_dir;
		std::string	filename; // utf8

		File_Or_Dir (bool is_dir): is_dir{is_dir} {}
		File_Or_Dir (bool is_dir, std::string filename): is_dir{is_dir}, filename{filename} {}
		virtual ~File_Or_Dir () {}
	};
	struct File : File_Or_Dir {
		File (std::string filename): File_Or_Dir{false, filename} {}
	};
	struct Directory : File_Or_Dir {
		Directory (): File_Or_Dir{true} {}
		Directory (std::string filename): File_Or_Dir{true, filename} {}

		bool	is_open = false; // dir needs to be open to be able to view files and subdirs in it (for performance reasons)
		bool	is_valid = true; // non open dirs are always "valid"

		std::vector<unique_ptr<File_Or_Dir>> files;

		void set_open (bool o, std::string const& path, std::string const& file_filter) {
			if (o == is_open) return;

			is_open = o;

			if (!is_open) {
				files.clear();
				is_valid = true;
			} else {
				std::vector<std::string> dirnames;
				std::vector<std::string> filenames;
				is_valid = n_find_files::find_files(path, &dirnames, &filenames, file_filter);
				if (is_valid) {
					for (auto& d : dirnames)
						files.emplace_back(make_unique<Directory>(d));
					for (auto& f : filenames)
						files.emplace_back(make_unique<File>(f));
				}
			}
		}
	};

	Directory						dir;
	unique_ptr<Directory_Watcher>	watcher = nullptr;

	std::string						file_filter;

	bool root_dir_is_valid () { return watcher != nullptr; }

	Dynamic_File_Tree () {}

	Dynamic_File_Tree (std::string const& dir_path, std::string const& file_filter) {
		dir = Directory(dir_path);
		watcher = false;
		this->file_filter = file_filter;

		watcher = make_unique<Directory_Watcher>(dir_path);
		if (!watcher->directory_is_valid())
			watcher = nullptr;
	}

	std::string const& get_path () {
		return dir.filename;
	}
	std::string const& get_file_filter () {
		return file_filter;
	}

	std::string get_parent_dir_path () {
		std::string dir = get_path();
		assert(dir.size() > 0 && dir[dir.size() -1] == '/');

		if (dir.size() == 3 && dir[1] == ':') // "C:/" -> "C:/"
			return dir;

		dir[dir.size() -1] = '#'; // get rid of last '/'

		auto slash_pos = dir.find_last_of('/');
		if (slash_pos == dir.npos)
			return "./";

		dir.resize(slash_pos +1);
		return dir;
	}
};

std::string path_from_pattern (std::string const& file_pattern, std::string* file_filter) {
	std::string path = file_pattern;

	bool prev_char_was_slash = false;
	for (auto it = path.begin(); it != path.end();) {
		bool erase_this_char = false;

		if (*it == '\\')
			*it = '/';

		if (*it == '/' && prev_char_was_slash)
			erase_this_char = true;

		prev_char_was_slash = *it == '/';

		if (erase_this_char)
			it = path.erase(it);
		else
			++it;
	}

	{
		std::string filter;

		auto pos = path.find_last_of('/');
		if (pos == path.npos) {
			filter = path;
			path = "";
		} else {
			filter = path.substr(pos +1);
			path.resize(pos);
		}

		if (filter.size() == 0)
			filter = "*";
		else
			filter = prints("*%s*", filter.c_str());

		*file_filter = filter;
	}

	if (path.size() == 0)
		path += '.';
	if (path.back() != '/')
		path += '/';

	return path;
}

struct Imgui_File_Browser {
	
	std::string			file_pattern;

	Dynamic_File_Tree	file_tree;

	bool				is_valid () { return file_tree.root_dir_is_valid(); };

	Imgui_File_Browser (std::string initial_file_pattern) {
		file_pattern = std::move(initial_file_pattern);
	}

	void show (Input& inp) {
		
		imgui::InputText_str("dir", &file_pattern);

		std::string file_filter;
		std::string path = path_from_pattern(file_pattern, &file_filter);

		bool folder_changed = file_tree.get_path() != path || file_tree.get_file_filter() != file_filter;
		if (folder_changed)
			file_tree = Dynamic_File_Tree(path, file_filter);

		if (folder_changed)
			printf("%s%s\n", path.c_str(), file_filter.c_str());

		bool valid = file_tree.root_dir_is_valid();
		imgui::SameLine();	imgui::TextColored(valid ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1), valid ? "OK" : "FAIL!");

		struct Recurse {
			Imgui_File_Browser* this_;
			bool folder_changed;
			Input& inp;
			
			void recurse (Dynamic_File_Tree::Directory* dir, std::string const& path, int depth = 0) {
				
				bool default_open = depth == 0;
				bool open = dir->is_open || (folder_changed && default_open);

				imgui::SetNextTreeNodeOpen(open);

				if (!dir->is_valid) imgui::PushStyleColor(ImGuiCol_Text, ImVec4(1,0,0,1));

				open = imgui::TreeNode(dir->filename.c_str());

				if (!dir->is_valid) imgui::PopStyleColor();

				dir->set_open(open, path, depth == 0 ? this_->file_tree.file_filter : "*"); // only apply filter on root dir

				if (depth == 0 && this_->file_tree.file_filter != "*") { // filter only counts on root dir
					imgui::SameLine();
					imgui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f,0.7f,0.3f,1));
					imgui::Text(prints("<%s>", this_->file_tree.file_filter.c_str()).c_str());
					imgui::PopStyleColor();
				}

				if (depth == 0) {
					imgui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5);

					imgui::SameLine();
					if (imgui::SmallButton(prints(" .. ###%s", dir->filename.c_str()).c_str())) { // exit directory
						this_->file_pattern = this_->file_tree.get_parent_dir_path();
					}

					imgui::PopStyleVar();
				} else {
					imgui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5);

					imgui::SameLine();
					if (imgui::SmallButton(prints(" -> ###%s", dir->filename.c_str()).c_str())) { // enter directory (make it root of of the Imgui_File_Tree)
						this_->file_pattern = path;
					}

					imgui::PopStyleVar();
				}

				//if (!dir.valid && imgui::IsItemHovered()) imgui::SetTooltip("reason for invalid dir");
				
				if (open) {	
					for (auto& f : dir->files) {
						if (f->is_dir) {
							auto* subdir = (Dynamic_File_Tree::Directory*)f.get();
							recurse(subdir, path + subdir->filename, depth +1);
						} else {
							auto* file = (Dynamic_File_Tree::File*)f.get();
							
							auto filepath = path + file->filename;

							bool selected = false;
							bool selected_changed = imgui::Selectable(file->filename.c_str(), &selected);

						}
					}
		
					if (dir->files.size() == 0) {
						imgui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f,0.3f,0.3f,1));
						imgui::Text("<empty>");
						imgui::PopStyleColor();
					}
		
					imgui::TreePop();
				}
			}
		};
		
		Recurse{this, folder_changed, inp}.recurse(&file_tree.dir, file_tree.dir.filename);

		/*
		
		struct Recurse {
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
		};

		*/
		//
		//auto sd = Show_Dir{inp};
		//sd.selected_file = selected_file;
		//
		//sd.selected_file_changed = false;
		//
		//sd.go_to_prev = inp.went_down_repeat(GLFW_KEY_LEFT);
		//sd.go_to_next = inp.went_down_repeat(GLFW_KEY_RIGHT);
		//
		//sd.recurse(dir, dir.name);
		//
		//return sd.selected_file_changed;
	}

	/*
	bool file_select (Input& inp, Texture2D** tex, iv2* size_px) {
		static bool init = true;
	
		static std::string folder = "J:/upscaler_test";
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
	
		static std::string selected_file = "J:/upscaler_test/01.jpg";
		bool selected_file_changed = Show_Dir::show(inp, dir, &selected_file);
	
		if (selected_file.size() == 0)
			*tex = nullptr;
		else
			*tex = texture_cacher.get_texture(selected_file, size_px);
	
		return selected_file_changed;
	}

	*/

};

}
