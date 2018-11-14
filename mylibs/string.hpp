#pragma once
#include <string>
#include <cstdarg>

#include "basic_typedefs.hpp"
#include "math.hpp"

namespace string {
	using namespace basic_typedefs;
	using namespace math;
	
	#define ANSI_COLOUR_CODE_RED	"\033[1;31m"
	#define ANSI_COLOUR_CODE_YELLOW	"\033[1;33m"
	#define ANSI_COLOUR_CODE_NC		"\033[0m"

	// Printf that outputs to a std::string
	inline void vprints (std::string* s, cstr format, va_list vl) { // print 
		size_t old_size = s->size();
		for (;;) {
			auto ret = vsnprintf(&(*s)[old_size], s->size() -old_size +1, format, vl); // i think i'm technically not allowed to overwrite the null terminator
			ret = MAX(0, ret);
			bool was_bienough = (u32)ret < (s->size() -old_size +1);
			s->resize(old_size +(u32)ret);
			if (was_bienough) break;
			// buffer was to small, buffer size was increased
			// now snprintf has to succeed, so call it again
		}
	}
	inline void prints (std::string* s, cstr format, ...) {
		va_list vl;
		va_start(vl, format);

		vprints(s, format, vl);

		va_end(vl);
	}
	inline std::string prints (cstr format, ...) {
		va_list vl;
		va_start(vl, format);

		std::string ret;
		vprints(&ret, format, vl);

		va_end(vl);

		return ret;
	}

	bool is_lower (char c) {
		return c >= 'a' && c <= 'z';
	}
	bool is_upper (char c) {
		return c >= 'A' && c <= 'Z';
	}

	char to_lower (char c) {
		return is_upper(c) ? c +('a' -'A') : c;
	}
	char to_upper (char c) {
		return is_lower(c) ? c +('A' -'a') : c;
	}

	std::basic_string<wchar_t> utf8_to_wchar (std::string const& utf8) {

		// overallocate, this might be more performant than having to process the utf8 twice
		std::basic_string<wchar_t> wstr (utf8.size() +1, '\0'); // wchar string can never be longer than number of utf8 bytes, right?
		
		auto res = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.c_str(), -1, &wstr[0], (int)wstr.size());
		assert(res > 0 && res <= wstr.size());

		wstr.resize(res -1);

		return wstr;
	}
	std::string wchar_to_utf8 (std::basic_string<wchar_t> const& wchar) {
		
		// overallocate, this might be more performant than having to process the wchar twice
		std::string utf8 (wchar.size() * 4 +1, '\0'); // utf8 string can never be longer than 4x the number of wchars, right?

		// WC_NO_BEST_FIT_CHARS sometimes throws erros ?
		auto res = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wchar.c_str(), -1, &utf8[0], (int)utf8.size(), NULL, NULL);
		auto err = GetLastError();
		assert(res > 0 && res <= utf8.size());

		utf8.resize(res -1);

		return utf8;

		/*
			Old code where i did a size determining pass first
				std::string filepath;

				auto required_size = WideCharToMultiByte(CP_UTF8, WC_NO_BEST_FIT_CHARS, info->FileName, -1, nullptr, 0, NULL, NULL);
				if (required_size == 0)
					break; // fail, do not continue reloading shaders

				assert(required_size >= 1); // required_size includes the null terminator
				filepath.resize(required_size);

				auto actual_size = WideCharToMultiByte(CP_UTF8, WC_NO_BEST_FIT_CHARS, info->FileName, -1, &filepath[0], (int)filepath.size(), NULL, NULL);
				if (actual_size != (int)filepath.size())
					break; // fail, do not continue reloading shaders

				filepath.resize(required_size -1); // remove redundant null terminator
				
		*/
	}
}
