#pragma once

#include "basic_typedefs.hpp"
#include "string.hpp"

#include <string>
#include <vector>

#define WIN32_LEAN_AND_MEAN 1
#include "windows.h"

namespace n_find_files {
	using namespace basic_typedefs;

	struct Directory {
		// contents
		std::vector<std::string>		dirnames;
		std::vector<std::string>		filenames;

	};
	struct Directory_Tree {
		std::string						name;
		// contents
		std::vector<Directory_Tree>		dirs;
		std::vector<std::string>		filenames;

		bool							valid; // could be file/dir not found, but also stuff like access denied

	};

	bool find_files (std::string const& dir_path, std::vector<std::string>* dirnames, std::vector<std::string>* filenames, std::string const& file_filter="*") { // strings can be utf8
		dirnames->clear();
		filenames->clear();

		WIN32_FIND_DATAW data;

		assert(dir_path.size() > 0 && dir_path.back() == '/');
		assert(file_filter.find_first_of('*') != file_filter.npos);

		std::string search_str = dir_path + file_filter;

		HANDLE hFindFile = FindFirstFileW(utf8_to_wchar(search_str).c_str(), &data);
		auto err = GetLastError();

		defer {
			FindClose(hFindFile);
		};

		if (hFindFile == INVALID_HANDLE_VALUE) {
			if (err == ERROR_FILE_NOT_FOUND) {
				// no file matches the pattern
				return true; // success
			} else if (err == ERROR_PATH_NOT_FOUND) {
				fprintf(stderr, "find_files: Could not find files in \"%s\" could not be found! FindFirstFile failed! [%x] (ERROR_PATH_NOT_FOUND)\n", search_str.c_str(), err);
			} else {
				fprintf(stderr, "find_files: Could not find files in \"%s\" could not be found! FindFirstFile failed! [%x]\n", search_str.c_str(), err);
			}
			return false; // fail
		}

		for (;;) {
			
			auto filename = wchar_to_utf8(std::basic_string<wchar_t>(data.cFileName));

			if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				if (	strcmp(filename.c_str(), ".") == 0 ||
						strcmp(filename.c_str(), "..") == 0 ) {
					// found directory represents the current directory or the parent directory, don't include this in the output
				} else {
					dirnames->emplace_back(std::move( filename +'/' ));
				}
			} else {
				filenames->emplace_back(filename);
			}

			auto ret = FindNextFileW(hFindFile, &data);
			auto err = GetLastError();
			if (ret == 0) {
				if (err == ERROR_NO_MORE_FILES) {
					break;
				} else {
					// TODO: in which cases would this happen?
					assert(false);
					fprintf(stderr, "find_files: FindNextFile failed! [%x]", err);

					dirnames->clear();
					filenames->clear();

					return false; // fail
				}
			}
		}

		return true; // success
	}

	// 
	bool find_files (std::string dir_path, Directory* dir, std::string const& file_filter="*") {
		if (!(dir_path.size() == 0 || dir_path.back() == '/'))
			dir_path.append("/");

		return find_files(dir_path, &dir->dirnames, &dir->filenames, file_filter);
	}

	bool find_files_recursive (std::string dir_path, std::string dir_name, Directory_Tree* dir, std::string const& file_filter="*") {
		if (!(dir_path.size() == 0 || dir_path.back() == '/'))
			dir_path.append("/");
		if (dir_name.back() != '/')
			dir_name.append("/");
		
		dir->name = dir_name;

		std::vector<std::string>	dirnames;

		std::string dir_full = dir_path+dir_name;

		dir->valid = find_files(dir_full, &dirnames, &dir->filenames, file_filter);

		dir->dirs.clear();

		for (auto& d : dirnames) {
			Directory_Tree subdir;
			find_files_recursive(dir_full, d, &subdir);
			dir->dirs.emplace_back( std::move(subdir) );
		}

		return dir->valid;
	}
	bool find_files_recursive (std::string const& dir_name, Directory_Tree* dir, std::string const& file_filter="*") {
		return find_files_recursive("", dir_name, dir, file_filter);
	}
}
