#pragma once

#include "windows.h"
#include "mylibs/string.hpp"
#include "mylibs/containers.hpp"

namespace n_directory_watcher {
//
using namespace basic_typedefs;
using namespace string;

enum file_change_e {
	FILE_ADDED,
	FILE_MODIFIED,
	FILE_RENAMED_NEW_NAME,
	FILE_REMOVED,
};

class Directory_Watcher {
	NO_MOVE_COPY_CLASS(Directory_Watcher)	// WARNING: Cannot copy or move

	std::string	directory_path;
	
	HANDLE		dir = INVALID_HANDLE_VALUE;
	OVERLAPPED	ovl = {};					// WARNING: Cannot copy or move, since a pointer to this passed into overlapped ReadDirectoryChangesW

	char buf[1024];							// WARNING: Cannot copy or move, since a pointer to this passed into overlapped ReadDirectoryChangesW

	bool watch_subdirs;

	bool do_ReadDirectoryChanges () {
		memset(buf, 0, sizeof(buf)); // ReadDirectoryChangesW does not null terminate the filenames?? This seems to fix the problem for now (filenames were overwriting each other aaa.txt overwrote the previous filename bbbbbbbbbbb.txt, which then read aaa.txtbbbb.txt)

		auto res = ReadDirectoryChangesW(dir, buf,sizeof(buf), watch_subdirs ? TRUE : FALSE,
											FILE_NOTIFY_CHANGE_FILE_NAME|
											FILE_NOTIFY_CHANGE_DIR_NAME|
											FILE_NOTIFY_CHANGE_SIZE|
											FILE_NOTIFY_CHANGE_LAST_WRITE|
											FILE_NOTIFY_CHANGE_CREATION,
											NULL, &ovl, NULL);
		if (!res) {
			auto err = GetLastError();
			if (err != ERROR_IO_PENDING)
				return false; // fail
		}
		return true;
	}

public:
	
	bool is_initialized () { return directory_path.size() != 0; }
	bool directory_is_valid () { return dir != INVALID_HANDLE_VALUE && ovl.hEvent != NULL; }
	
	Directory_Watcher (std::string const& directory_path, bool watch_subdirs=true) { // must end in slash, since is is prepended in front of the filenames, so that changed_files contains "directory_path/subdir/filename"
		this->directory_path = directory_path;

		dir = CreateFileW(utf8_to_wchar(directory_path).c_str(), FILE_LIST_DIRECTORY, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OVERLAPPED, NULL);
		auto dir_err = GetLastError();

		ovl.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

		if (dir == INVALID_HANDLE_VALUE || ovl.hEvent == NULL) {
			fprintf(stderr, "Directory_Watcher init failed with directory_path=\"%s\" (%s), won't monitor file changes!\n", directory_path.c_str(), dir_err == ERROR_FILE_NOT_FOUND ? "ERROR_FILE_NOT_FOUND" : "unknown error");
		}

		do_ReadDirectoryChanges();
	}

	~Directory_Watcher () {
		if (dir != INVALID_HANDLE_VALUE)
			CloseHandle(dir);
		if (ovl.hEvent != NULL)
			CloseHandle(ovl.hEvent);
	}

	struct File_Change {
		std::string		filepath;
		file_change_e	change;
	};
	bool poll_file_changes (std::vector<File_Change>* changed_files=nullptr) {
		if (!directory_is_valid())
			return false;

		DWORD bytes_returned;
		auto res = GetOverlappedResult(dir, &ovl, &bytes_returned, FALSE);
		if (!res) {
			auto err = GetLastError();
			if (err != ERROR_IO_INCOMPLETE) {
				// Error ?
			}
			return false;
		} else {
			
			std::vector<File_Change> tmp_changed_files;
			if (!changed_files)
				changed_files = &tmp_changed_files;

			char const* cur = buf;

			for (;;) {
				auto remaining_bytes = (uptr)bytes_returned -(uptr)(cur -buf);
				if (remaining_bytes == 0)
					break; // all changes processed

				assert(remaining_bytes >= sizeof(FILE_NOTIFY_INFORMATION));
				FILE_NOTIFY_INFORMATION* info = (FILE_NOTIFY_INFORMATION*)cur;

				assert(remaining_bytes >= offsetof(FILE_NOTIFY_INFORMATION, FileName) +info->FileNameLength); // bytes_returned does not include a null terminator after info->FileName ??

				std::string filepath = wchar_to_utf8(info->FileName);

				#if 1
				cstr action_str = nullptr;
				switch (info->Action) {
					case FILE_ACTION_ADDED:				action_str = "FILE_ACTION_ADDED             ";	break;
					case FILE_ACTION_MODIFIED:			action_str = "FILE_ACTION_MODIFIED          ";	break;
					case FILE_ACTION_RENAMED_NEW_NAME:	action_str = "FILE_ACTION_RENAMED_NEW_NAME  ";	break;
					case FILE_ACTION_REMOVED:			action_str = "FILE_ACTION_REMOVED           ";	break;
					case FILE_ACTION_RENAMED_OLD_NAME:	action_str = "FILE_ACTION_RENAMED_OLD_NAME  ";	break;

					default:							action_str = "unknown"; break;
				}

				printf("%s to \"%s\" detected.\n", action_str, filepath.c_str());
				#endif

				file_change_e mode;

				switch (info->Action) {
					case FILE_ACTION_ADDED:				mode = FILE_ADDED;				break;
					case FILE_ACTION_MODIFIED:			mode = FILE_MODIFIED;			break;
					case FILE_ACTION_RENAMED_NEW_NAME:	mode = FILE_RENAMED_NEW_NAME;	break;
					case FILE_ACTION_REMOVED:			mode = FILE_REMOVED;			break;
				}

				switch (info->Action) {
					case FILE_ACTION_ADDED:				// file was added, report it
					case FILE_ACTION_MODIFIED:			// file was modified, report it
					case FILE_ACTION_RENAMED_NEW_NAME:	// file was renamed and this is the new name (its like a file with the new name was added), report it
					case FILE_ACTION_REMOVED:			// file was deleted, report it
					{
						// all assets with dependencies on this file should be reloaded
						
						if (filepath.find_first_of('~') != std::string::npos) { // string contains a tilde '~' character
							// tilde characters are often used for temporary files, for ex. MSVC writes source code changes by creating a temp file with a tilde in it's name and then swaps the old and new file by renaming them
							//  so filter those files here since the user of Directory_Watcher _probably_ does not want those files
						} else {
							filepath.insert(0, directory_path);
							
							changed_files->push_back( File_Change{ std::move(filepath), mode } );
						}
					} break;

					case FILE_ACTION_RENAMED_OLD_NAME:	// file was renamed and this is the old name (its like the file with the old name was deleted), don't report it
					default:
						// do not try to reload assets with dependencies on files that are deleted or renamed (old name no longer exists), instead just keep the asset loaded
						break;
				}

				if (info->NextEntryOffset == 0)
					break; // all changes processed

				cur += info->NextEntryOffset;
			}

			ResetEvent(ovl.hEvent);

			do_ReadDirectoryChanges();

			return changed_files->size() != 0;
		}
	}

	bool poll_file_changes_ignore_removed (std::vector<std::string>* changed_files=nullptr) {
		std::vector<File_Change> changes;
		bool res = poll_file_changes(&changes);

		changed_files->clear();
		for (auto& c : changes)
			if (c.change != FILE_REMOVED && !contains(*changed_files, c.filepath)) // do not report a file as changed twice
				changed_files->push_back(c.filepath);

		return res;
	}
};

//
}
using namespace n_directory_watcher;
