#pragma once

#include "engine_include.hpp"
#include <algorithm>

namespace engine {
	
	template <typename T, typename PRED>	T const* find_first (std::vector<T> const& v, PRED pred) {
		for (sptr i=0; i<(sptr)v.size(); ++i)
			if (pred(v[i]))
				return &v[i];
		return nullptr;
	}
	template <typename T, typename PRED>	T const* find_last (std::vector<T> const& v, PRED pred) {
		for (sptr i=(sptr)v.size()-1; i>=0; --i)
			if (pred(v[i]))
				return &v[i];
		return nullptr;
	}

	template <typename T, typename PRED>	T* find_first (std::vector<T>& v, PRED pred) {
		for (sptr i=0; i<(sptr)v.size(); ++i)
			if (pred(v[i]))
				return &v[i];
		return nullptr;
	}
	template <typename T, typename PRED>	T* find_last (std::vector<T>& v, PRED pred) {
		for (sptr i=(sptr)v.size()-1; i>=0; --i)
			if (pred(v[i]))
				return &v[i];
		return nullptr;
	}

	// Have a imgui button that opens windows of type T (calls imgui() on them) when clicked
	// T: struct with method  void imgui ();  and  bool open;  and  static cstr window_name;
	template <typename T>
	struct Window_Button {
		std::vector<T>	windows;

		template <typename USER_T>
		void imgui (USER_T* user_ptr=nullptr) {
			
			if (imgui::Button(T::window_name)) {
				auto* w = find_first(windows, [] (T& w) { return !w.open; });

				if (!w) {
					windows.push_back(T());
					w = &windows.back();
				} else {
					*w = T();
				}

				w->open = true;
			}

			int i = 0;
			for (auto& w : windows) {
				if (w.open && imgui::Begin(prints("%s #%d", T::window_name, i +1).c_str(), &w.open)) {
					
					w.imgui(user_ptr);

					imgui::End();
				}
				i++;
			}

			auto last = find_last(windows, [] (T& w) { return w.open; });
			if (last)
				windows.resize(last +1 -&windows[0]); // remove closed windows at the end (keep the ones in the middle, so that their indexes stay the same for imgui)
		}
	};

}
