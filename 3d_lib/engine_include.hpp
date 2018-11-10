#pragma once

#define _CRT_SECURE_NO_WARNINGS

// language includes
#include <string>
#include <vector>
#include <cassert>

#include <memory>
using std::unique_ptr;
using std::make_unique;
using std::shared_ptr;
using std::make_shared;
//

// library includes
#include "deps/glad/glad.c"

#include "deps/dear_imgui/imgui.h"

#include "deps/glfw/include/GLFW/glfw3.h"

#pragma push_macro("APIENTRY")

#define GLFW_EXPOSE_NATIVE_WIN32 1
#include "deps/glfw/include/GLFW/glfw3native.h"

#pragma pop_macro("APIENTRY")

#include "windows.h"
#include "Shlobj.h"
//

// my includes
#include "mylibs/basic_typedefs.hpp"
#include "mylibs/vector.hpp"
#include "mylibs/simple_file_io.hpp"
#include "mylibs/string.hpp"

static constexpr bool not_implemented = false; // use like: assert(not_implemented)
											   //

#include "mylibs/colors.hpp"
#include "mylibs/float_precision.hpp"

#define errprint(...) fprintf(stderr, __VA_ARGS__)

namespace engine {
	using namespace basic_typedefs;
	using namespace vector;
	using namespace float_precision;
	using namespace colors;
	using namespace string;

	struct Screen_Rect {
		iv2		offs_px;
		iv2		size_px;
	};

	struct Input {

		bool gui_input_enabled = true;

		bool block_mouse = false;
		bool block_keyboard = false;
		bool blocked_by_typing = false;

		void reset_blocked () {
			block_mouse = false;
			block_keyboard = false;
			blocked_by_typing = false;
		}

		iv2	wnd_size_px;

		Screen_Rect get_window_screen_rect () const {
			return { 0, wnd_size_px };
		}

		struct Mousecursor {
			v2		pos_screen; // top-down
			v2		delta_screen;
		} mousecursor;

		v2 mouse_cursor_pos_px () { // bottom up, pixel center
			v2 p = mousecursor.pos_screen;
			return v2(p.x, (flt)wnd_size_px.y -1 -p.y) + 0.5f;
		}
		v2 mouse_cursor_pos_ndc () { // bottom up, pixel center
			return mouse_cursor_pos_px() / (v2)wnd_size_px * 2 -1;
		}

		v2 mouse_cursor_pos_px (Screen_Rect const& rect) { // bottom up, pixel center
			return mouse_cursor_pos_px() -(v2)rect.offs_px;
		}
		v2 mouse_cursor_pos_ndc (Screen_Rect const& rect) { // bottom up, pixel center
			return mouse_cursor_pos_px(rect) / (v2)rect.size_px * 2 -1;
		}

		bool mouse_curson_in (Screen_Rect const& rect) {
			v2 pos = mouse_cursor_pos_px() -(v2)rect.offs_px;
			return all(pos >= 0 && pos < (v2)rect.size_px);
		}

		struct Mousewheel {
			flt		delta;
		} _mousewheel;

		flt get_mousewheel_delta () {
			if (block_mouse) return 0;
			return _mousewheel.delta;
		}

		struct Button {
			bool	is_down		: 1;
			bool	went_down	: 1;
			bool	went_up		: 1;
			bool	os_repeat	: 1;
		};

		Button _buttons[GLFW_KEY_LAST +1] = {}; // lower 8 indecies are used as mouse button (GLFW_MOUSE_BUTTON_1 - GLFW_MOUSE_BUTTON_8), glfw does not seem to have anything assigned to them

		bool is_down (int glfw_button) const {
			if (block_keyboard) return false;
			return _buttons[glfw_button].is_down;
		}
		bool went_down (int glfw_button) const {
			if (block_keyboard) return false;
			return _buttons[glfw_button].went_down;
		}
		bool went_up (int glfw_button) const {
			if (block_keyboard) return false;
			return _buttons[glfw_button].went_up;
		}
		bool went_down_repeat (int glfw_button) const {
			if (block_keyboard) return false;
			return _buttons[glfw_button].went_down || _buttons[glfw_button].os_repeat;
		}

		bool key_combo (int glfw_mod_key_l, int glfw_mod_key_r, int glfw_key) {
			if (blocked_by_typing) return false; // still trigger ctrl+key shortcut if keyboard is blocked, since key does not interfere with imgui (don't trigger during text typing)

			auto& lc = _buttons[glfw_mod_key_l];
			auto& rc = _buttons[glfw_mod_key_r];
			auto& key = _buttons[glfw_key];

			return	( (lc.is_down ||   rc.is_down) &&  key.is_down ) && // all keys in combo must be down
					(  lc.went_down || rc.went_down || key.went_down ); // but only trigger if one of them went down
		}
		bool ctrl_combo (int glfw_key) {	return key_combo(GLFW_KEY_LEFT_CONTROL, GLFW_KEY_RIGHT_CONTROL, glfw_key); }
		bool alt_combo (int glfw_key) {		return key_combo(GLFW_KEY_LEFT_ALT, GLFW_KEY_RIGHT_ALT, glfw_key); }

		struct Event {

			enum type_e {
				MOUSEMOVE,
				MOUSEWHEEL,
				BUTTON, // Button transition, no repeats
				TYPING,
			};

			type_e	type;

			union {
				struct {
					v2			delta_screen;
				} Mousemove;
				struct {
					v2			delta;
				} Mousewheel;
				struct {
					int			glfw_button;
					bool		went_down;
				} Button;
				struct {
					utf32		codepoint;
				} Typing;
			};

			Event () {} 
			Event (type_e t): type{t} {} 
		};

		std::vector<Event>	events;
	};

}
