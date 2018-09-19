#pragma once

#include <string>

#include "stdio.h"

#include "basic_typedefs.hpp"
#include "defer.hpp"

namespace simple_file_io {
	using namespace basic_typedefs;

	bool load_text_file (cstr filepath, std::string* text) {

		FILE* f = fopen(filepath, "rb"); // i don't want "\r\n" to "\n" conversion, because it interferes with my file size calculation and i usually handle \r\n anyway
		if (!f)
			return false;
		defer {
			fclose(f);
		};

		fseek(f, 0, SEEK_END);
		int filesize = ftell(f);
		fseek(f, 0, SEEK_SET);

		text->resize(filesize);

		uptr ret = fread(&(*text)[0], 1,text->size(), f);
		if (ret != (uptr)filesize) return false;

		return true;
	}

	class Blob {
		MOVE_ONLY_CLASS(Blob)

	public:
		void*	data = nullptr;
		uptr	size = 0;

		//
		static Blob alloc (uptr size) {
			Blob b;
			b.data = malloc(size);
			b.size = size;
			return b;
		}

		~Blob () {
			if (data)
				free(data);
		}
	};
	void swap (Blob& l, Blob& r) {
		std::swap(l.data, r.data);
		std::swap(l.size, r.size);
	}

	bool load_binary_file (cstr filepath, Blob* blob) {

		FILE* f = fopen(filepath, "rb"); // we don't want "\r\n" to "\n" conversion
		if (!f)
			return false;
		defer {
			fclose(f);
		};

		fseek(f, 0, SEEK_END);
		int filesize = ftell(f);
		fseek(f, 0, SEEK_SET);

		auto tmp = Blob::alloc(filesize);

		uptr ret = fread(tmp.data, 1,tmp.size, f);
		if (ret != (uptr)filesize) return false;

		*blob = std::move(tmp);
		return true;
	}

	bool load_fixed_size_binary_file (cstr filepath, void* data, uptr sz) {

		FILE* f = fopen(filepath, "rb");
		if (!f)
			return false;
		defer {
			fclose(f);
		};

		fseek(f, 0, SEEK_END);
		int filesize = ftell(f);
		fseek(f, 0, SEEK_SET);

		if (filesize != sz)
			return false;

		uptr ret = fread(data, 1,sz, f);
		if (ret != sz)
			return false;

		return true;
	}

	bool write_fixed_size_binary_file (cstr filepath, void const* data, uptr sz) {

		FILE* f = fopen(filepath, "wb");
		if (!f)
			return false;
		defer {
			fclose(f);
		};

		uptr ret = fwrite(data, 1,sz, f);
		if (ret != sz)
			return false;

		return true;
	}

	bool write_text_file (cstr filepath, std::string const& text) {
		return write_fixed_size_binary_file(filepath, &text[0], text.size());
	}
}
