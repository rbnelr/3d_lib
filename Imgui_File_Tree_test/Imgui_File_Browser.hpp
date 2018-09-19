#pragma once

#include "3d_lib/engine.hpp"
#include "mylibs/find_files.hpp"

namespace imgui_file_browser {
using namespace engine;

struct Dynamic_File_Tree {
	
	struct File_Or_Dir {
		bool		is_dir;
		std::string	filename; // utf8

		File_Or_Dir (bool d): is_dir{d} {}
		File_Or_Dir (bool d, std::string fn): is_dir{d}, filename{fn} {}
		virtual ~File_Or_Dir () {}
	};
	struct File : File_Or_Dir {
		File (std::string filename): File_Or_Dir{false, filename} {}
	};
	struct Directory : File_Or_Dir {
		Directory (): File_Or_Dir{true} {}
		Directory (std::string filename): File_Or_Dir{true, filename} {}

		bool	is_open = false; // dir needs to be open to be able to view files and subdirs in it (for performance reasons)
		bool	is_valid = true; // os can throw errors on file queries -> invalid dir (no files and subdirs), non open dirs are always "valid"

		std::vector<shared_ptr<Directory>>	dirs;
		std::vector<shared_ptr<File>>		files;

		unique_ptr<Directory_Watcher>		watcher = nullptr;

		bool is_empty () {
			return dirs.size() == 0 && files.size() == 0;
		}

		std::vector<shared_ptr<File_Or_Dir>> get_files (bool sort_alphabethical, bool subdirs_always_first) {
			std::vector<shared_ptr<File_Or_Dir>> list;
			for (auto& d : dirs)	list.push_back(d);
			for (auto& f : files)	list.push_back(f);

			std::sort(list.begin(),list.end(), [&] (shared_ptr<File_Or_Dir> const& l, shared_ptr<File_Or_Dir> const& r) {
				if (subdirs_always_first && (l->is_dir != r->is_dir))
					return l->is_dir;
				return std::less<std::string>()(l->filename, r->filename);
			});

			return list;
		}

		void update (std::string const& path, std::string const& new_dirname, std::string const& file_filter) {
			
			auto old_dirs	= std::move(dirs);
			auto old_files	= std::move(files);

			if (filename != new_dirname) {
				old_dirs.clear();
				old_files.clear();
			}
			filename = new_dirname;

			if (!is_open)
				return; // dirs/files are empty

			std::string dirpath = path + filename;

			std::vector<std::string> dirnames;
			std::vector<std::string> filenames;
			is_valid = n_find_files::find_files(dirpath, &dirnames, &filenames, file_filter);

			watcher = make_unique<Directory_Watcher>(dirpath);

			if (is_valid) {
				
				// keep old Directory and File structures alive if we have the same dir/file in the old and the updated file tree -> this keeps directories open after a reload
				for (auto& subdirname : dirnames) {
					auto res = std::find_if(old_dirs.begin(),old_dirs.end(), [&] (shared_ptr<Directory>& d) { return d && d->filename == subdirname; });
					auto dir = res != old_dirs.end() ? std::move(*res) : nullptr;

					if (!dir)
						dir = make_shared<Directory>(subdirname);
					
					dir->update(dirpath, subdirname, file_filter);

					dirs.emplace_back(std::move(dir));
				}
				for (auto& filename : filenames) {
					auto res = std::find_if(old_files.begin(),old_files.end(), [&] (shared_ptr<File>& f) { return f && f->filename == filename; });
					auto file = res != old_files.end() ? std::move(*res) : nullptr;

					if (!file)
						file = make_shared<File>(filename);
					
					files.emplace_back(std::move(file));
				}

			}
		}

		void poll_files_changed (std::string const& path, std::string const& new_filename, std::string const& file_filter) {
			
			if (!is_open)
				return;

			std::vector<Directory_Watcher::File_Change> file_changes;
			bool files_changed = watcher->poll_file_changes(&file_changes);

			std::string dirpath = path + new_filename;

			if (files_changed) {

				for (auto& dir : dirs)
					dir->update(dirpath, dir->filename, file_filter);
			} else {
				
				for (auto& dir : dirs)
					dir->poll_files_changed(dirpath, dir->filename, file_filter);
			}

		}

		void set_open (bool o, std::string const& path, std::string const& file_filter) {
			if (o == is_open) return;

			is_open = o;

			if (!is_open) {
				dirs.clear();
				files.clear();
				watcher = nullptr;

				is_valid = true;
			} else {
				update(path, filename, file_filter);
			}
		}
	};

	std::string				base_path;

	shared_ptr<Directory>	root_dir = make_shared<Directory>();

	std::string				file_filter;

	bool root_dir_is_valid () { return root_dir->is_valid; }

	void update (std::string const& new_dirname) {
		root_dir->update(base_path, new_dirname, file_filter);
	}
	void poll_files_changed (std::string const& dirname) {
		root_dir->poll_files_changed(base_path, dirname, file_filter);
	}

	std::string const& get_root_dirname () {
		return root_dir->filename;
	}

	static void split_path (std::string path, std::string* basepath, std::string* dirname) {
		assert(path.size() > 0 && path.back() != '/');

		auto pos = path.find_last_of('/');
		if (pos == path.npos) {

			*basepath = "";
			*dirname = std::move(path);

		} else {

			*dirname = path.substr(pos +1);

			path.resize(pos +1);
			*basepath = std::move(path);
		}

		if (dirname->size() == 0)
			*dirname = ".";
		if (dirname->back() != '/')
			*dirname += '/';
	}
	static void path_from_pattern (std::string const& file_pattern, std::string* basepath, std::string* dirname, std::string* file_filter) {
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

			*file_filter = std::move(filter);
		}

		split_path(std::move(path), basepath, dirname);
	}
	void get_parent_dir_path (std::string path, std::string* parent_path, std::string* dirname) {
		assert(path.size() > 0 && path[path.size() -1] == '/');
		path.resize( path.size() -1 );

		split_path(std::move(path), parent_path, dirname);
	}
	
	void make_dir_root_dir (shared_ptr<Directory>& dir, std::string const& new_basepath) {
		base_path = new_basepath;
		root_dir = dir;
	}
	void make_parent_dir_root_dir () {

		assert(has_parent_dir());
			
		std::string new_root_dirname;
		std::string new_base_path;

		get_parent_dir_path(std::move(base_path), &new_base_path, &new_root_dirname);

		auto new_root_dir = make_shared<Directory>(new_root_dirname);
		new_root_dir->dirs.push_back( root_dir );
		new_root_dir->is_open = true; // need to set this to open or else the existing subdirs will be thrown away and recreated

		base_path = std::move(new_base_path);
		root_dir = new_root_dir;
	}

	bool has_parent_dir () { return base_path.size() > 0; };
	
};


struct Imgui_File_Browser {
	
	std::string			file_pattern;

	Dynamic_File_Tree	file_tree;

	bool				is_valid () { return file_tree.root_dir_is_valid(); };

	bool				trigger_folder_changed;

	Imgui_File_Browser (std::string initial_file_pattern) {
		file_pattern = std::move(initial_file_pattern);
	}

	void show (Input& inp) {
		
		imgui::InputText_str("###dir", &file_pattern);
		{
			bool valid = file_tree.root_dir_is_valid();
			imgui::SameLine();	imgui::TextColored(valid ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1), valid ? "OK" : "FAIL!");
		}

		std::string new_basepath, new_dirname, new_file_filter;
		Dynamic_File_Tree::path_from_pattern(file_pattern, &new_basepath, &new_dirname, &new_file_filter);

		std::vector<std::string> changed_files;

		imgui::SameLine();
		bool folder_changed =	imgui::Button("Reload")
							||	trigger_folder_changed
							||	file_tree.base_path				!= new_basepath
							||	file_tree.get_root_dirname()	!= new_dirname
							||	file_tree.file_filter			!= new_file_filter
				//(watcher && watcher->poll_file_changes(&changed_files))
				; // directory watcher reports changes in all subdirs (even if we dont have those folders open), but just update the file tree anyway for now
		if (folder_changed) {
			file_tree.base_path = new_basepath;
			file_tree.file_filter = new_file_filter;
			file_tree.update(new_dirname);
			
			//assert(file_tree.root_dir_is_valid() == watcher->directory_is_valid());
		}

		file_tree.poll_files_changed(new_dirname);

		trigger_folder_changed = false;

		if (new_file_filter != "*" && inp._buttons[GLFW_KEY_TAB].went_down) {
			if (file_tree.root_dir->dirs.size() > 0) {
				file_tree.make_dir_root_dir(file_tree.root_dir->dirs[0], file_tree.base_path + file_tree.get_root_dirname());

				file_pattern = file_tree.base_path + file_tree.get_root_dirname();
				trigger_folder_changed = true;

				// TODO: imgui keeps overwriting our cahnge of the file_pattern
			}
		}

		struct Recurse {
			Imgui_File_Browser* this_;
			bool	folder_changed;
			bool*	trigger_folder_changed;
			Input& inp;
			
			void recurse (shared_ptr<Dynamic_File_Tree::Directory> dir, std::string const& path, int depth = 0) {
				
				bool default_open = depth == 0;
				bool open = dir->is_open || (default_open && folder_changed);

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
					if (this_->file_tree.has_parent_dir()) {
						imgui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5);

						imgui::SameLine();
						if (imgui::SmallButton(prints(" .. ###%s", dir->filename.c_str()).c_str())) { // exit directory
							this_->file_tree.make_parent_dir_root_dir();

							this_->file_pattern = this_->file_tree.base_path + this_->file_tree.get_root_dirname();
							*trigger_folder_changed = true;
						}

						imgui::PopStyleVar();
					}
				} else {
					imgui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5);

					imgui::SameLine();
					if (imgui::SmallButton(prints(" -> ###%s", dir->filename.c_str()).c_str())) { // enter directory (make it root of of the Imgui_File_Tree)
						this_->file_tree.make_dir_root_dir(dir, path);

						this_->file_pattern = this_->file_tree.base_path + this_->file_tree.get_root_dirname();
						*trigger_folder_changed = true;
					}

					imgui::PopStyleVar();
				}

				//if (!dir.valid && imgui::IsItemHovered()) imgui::SetTooltip("reason for invalid dir");
				
				if (open) {	
					for (auto& f : dir->get_files(false, true)) {
						if (f->is_dir) {
							auto subdir = std::static_pointer_cast<Dynamic_File_Tree::Directory>(f);
							recurse(subdir, path + dir->filename, depth +1);
						} else {
							auto file = std::static_pointer_cast<Dynamic_File_Tree::File>(f);
							
							auto filepath = path + file->filename;

							bool selected = false;
							bool selected_changed = imgui::Selectable(file->filename.c_str(), &selected);

						}
					}
		
					if (dir->is_empty()) {
						imgui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f,0.3f,0.3f,1));
						imgui::Text("<empty>");
						imgui::PopStyleColor();
					}
		
					imgui::TreePop();
				}
			}
		};
		
		Recurse{this, folder_changed, &trigger_folder_changed, inp}.recurse(file_tree.root_dir, file_tree.base_path);

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
