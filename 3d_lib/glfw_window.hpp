#pragma once

#include "engine_include.hpp"
#include "dear_imgui.hpp"
#include "gl_rendertarget.hpp"
#include "mylibs/timer.hpp"
#include "mylibs/exp_moving_avg.hpp"

#include "save_file.hpp"

namespace engine {
	using namespace simple_file_io;

	// for now
	Save* save;
	
	//
	void APIENTRY ogl_debuproc (GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, cstr message, void const* userParam) {

		//if (source == GL_DEBUG_SOURCE_SHADER_COMPILER_ARB) return;

		// hiding irrelevant infos/warnings
		switch (id) {
			case 131185: // Buffer detailed info (where the memory lives which is supposed to depend on the usage hint)
						 //case 1282: // using shader that was not compiled successfully
						 //
						 //case 2: // API_ID_RECOMPILE_FRAGMENT_SHADER performance warning has been generated. Fragment shader recompiled due to state change.
						 //case 131218: // Program/shader state performance warning: Fragment shader in program 3 is being recompiled based on GL state.
						 //
						 //			 //case 131154: // Pixel transfer sync with rendering warning
						 //
						 //			 //case 1282: // Wierd error on notebook when trying to do texture streaming
						 //			 //case 131222: // warning with unused shadow samplers ? (Program undefined behavior warning: Sampler object 0 is bound to non-depth texture 0, yet it is used with a program that uses a shadow sampler . This is undefined behavior.), This might just be unused shadow samplers, which should not be a problem
						 //			 //case 131218: // performance warning, because of shader recompiling based on some 'key'
				return;
		}

		cstr src_str = "<unknown>";
		switch (source) {
			case GL_DEBUG_SOURCE_API_ARB:				src_str = "GL_DEBUG_SOURCE_API_ARB";				break;
			case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB:		src_str = "GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB";		break;
			case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB:	src_str = "GL_DEBUG_SOURCE_SHADER_COMPILER_ARB";	break;
			case GL_DEBUG_SOURCE_THIRD_PARTY_ARB:		src_str = "GL_DEBUG_SOURCE_THIRD_PARTY_ARB";		break;
			case GL_DEBUG_SOURCE_APPLICATION_ARB:		src_str = "GL_DEBUG_SOURCE_APPLICATION_ARB";		break;
			case GL_DEBUG_SOURCE_OTHER_ARB:				src_str = "GL_DEBUG_SOURCE_OTHER_ARB";				break;
		}

		cstr type_str = "<unknown>";
		switch (source) {
			case GL_DEBUG_TYPE_ERROR_ARB:				type_str = "GL_DEBUG_TYPE_ERROR_ARB";				break;
			case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB:	type_str = "GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB";	break;
			case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB:	type_str = "GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB";	break;
			case GL_DEBUG_TYPE_PORTABILITY_ARB:			type_str = "GL_DEBUG_TYPE_PORTABILITY_ARB";			break;
			case GL_DEBUG_TYPE_PERFORMANCE_ARB:			type_str = "GL_DEBUG_TYPE_PERFORMANCE_ARB";			break;
			case GL_DEBUG_TYPE_OTHER_ARB:				type_str = "GL_DEBUG_TYPE_OTHER_ARB";				break;
		}

		cstr severity_str = "<unknown>";
		switch (severity) {
			case GL_DEBUG_SEVERITY_HIGH_ARB:			severity_str = "GL_DEBUG_SEVERITY_HIGH_ARB";		break;
			case GL_DEBUG_SEVERITY_MEDIUM_ARB:			severity_str = "GL_DEBUG_SEVERITY_MEDIUM_ARB";		break;
			case GL_DEBUG_SEVERITY_LOW_ARB:				severity_str = "GL_DEBUG_SEVERITY_LOW_ARB";			break;
		}

		fprintf(stderr, "OpenGL debug proc: severity: %s src: %s type: %s id: %d  %s\n",
				severity_str, src_str, type_str, id, message);
	}

	struct RectI {
		iv2	low;
		iv2	high;

		inline iv2 get_size () {
			return high -low;
		}
	};

	RectI to_rect (RECT win32_rect) {
		return {	iv2(win32_rect.left,	win32_rect.top),
			iv2(win32_rect.right,	win32_rect.bottom) };
	}

	enum e_vsync_mode {
		VSYNC_ON,
		VSYNC_OFF,
	};

	class Window {
		friend class Application;

		GLFWwindow*			window = nullptr;

		e_vsync_mode		vsync_mode;

		WINDOWPLACEMENT		win32_windowplacement = {}; // Zeroed

		bool				is_fullscreen = false;
		iv2					fullscreen_resolution = -1; // -1 => native res

		void get_win32_windowplacement () {
			GetWindowPlacement(glfwGetWin32Window(window), &win32_windowplacement);
		}
		void set_win32_windowplacement () {
			SetWindowPlacement(glfwGetWin32Window(window), &win32_windowplacement);
		}
		
		void save_window_positioning () {

			if (is_fullscreen) {
				// keep window positioning that we got when we switched to fullscreen
			} else {
				get_win32_windowplacement();
			}

			if (!write_fixed_size_binary_file("saves/window_placement.bin", &win32_windowplacement, sizeof(win32_windowplacement))) {
				fprintf(stderr, "Could not save window_placement to saves/window_placement.bin, window position and size won't be restored on the next launch of this app.");
			}
		}
		bool load_window_positioning () {
			return load_fixed_size_binary_file("saves/window_placement.bin", &win32_windowplacement, sizeof(win32_windowplacement));
		}

		static void glfw_error_proc (int err, cstr msg) {
			fprintf(stderr, "GLFW Error! 0x%x '%s'\n", err, msg);
		}
		static void button_event (GLFWwindow* window, int button, int action, int mods) {
			Input* inp = &((Window*)glfwGetWindowUserPointer(window))->inp;

			bool went_down = action == GLFW_PRESS;
			bool went_up = action == GLFW_RELEASE;
			bool repeat = action == GLFW_REPEAT;

			if (!(button >= 0 && button < ARRLEN(Input::_buttons)))
				return;

			assert(went_down || went_up || repeat);
			if (!(went_down || went_up || repeat))
				return;

			inp->events.push_back({ Input::Event::BUTTON });
			inp->events.back().Button.glfw_button = button;
			inp->events.back().Button.went_down = went_down;

			inp->_buttons[button].is_down =	went_down || repeat;
			inp->_buttons[button].went_down =	went_down;
			inp->_buttons[button].went_up =	went_up;
			inp->_buttons[button].os_repeat =	repeat;
		}

		static void glfw_mouse_pos_event (GLFWwindow* window, double xpos, double ypos) {
			Input* inp = &((Window*)glfwGetWindowUserPointer(window))->inp;

			v2 new_pos = v2((flt)xpos,(flt)ypos);
			static v2 prev_pos;

			static bool first_call = true;
			if (!first_call) {

				v2 diff = new_pos -prev_pos;
				inp->mousecursor.delta_screen += diff;

			}
			first_call = false;

			prev_pos = new_pos;
		}
		static void glfw_mouse_button_event (GLFWwindow* window, int button, int action, int mods) {
			button_event(window, button, action, mods);
		}
		static void glfw_mouse_scroll (GLFWwindow* window, double xoffset, double yoffset) {
			Input* inp = &((Window*)glfwGetWindowUserPointer(window))->inp;

			inp->_mousewheel.delta += (flt)yoffset;

			inp->events.push_back({ Input::Event::MOUSEWHEEL });
			inp->events.back().Mousewheel.delta = (flt)yoffset;
		}
		static void glfw_key_event (GLFWwindow* window, int key, int scancode, int action, int mods) {
			button_event(window, key, action, mods);
		}
		static void glfw_char_event (GLFWwindow* window, unsigned int codepoint, int mods) {
			Input* inp = &((Window*)glfwGetWindowUserPointer(window))->inp;

			inp->events.push_back({ Input::Event::TYPING });
			inp->events.back().Typing.codepoint = (utf32)codepoint;
		}
		
	public:

		Input				inp;

		void set_fullscreen (bool want_fullscreen) {
			if (!is_fullscreen && !want_fullscreen) return; // going from windowed to windowed

			bool was_windowed = !is_fullscreen;

			if (want_fullscreen) {

				if (was_windowed)
					get_win32_windowplacement();

				GLFWmonitor* fullscreen_monitor = nullptr;
				GLFWvidmode const* fullscreen_vidmode = nullptr;
				{
					int count;
					GLFWmonitor** monitors = glfwGetMonitors(&count);

					flt				min_dist = INF;

					iv2 wnd_pos;
					iv2 wnd_sz;
					glfwGetWindowPos(window, &wnd_pos.x,&wnd_pos.y);
					glfwGetWindowSize(window, &wnd_sz.x,&wnd_sz.y);

					v2 wnd_center = ((v2)wnd_pos +(v2)wnd_sz) / 2;

					for (int i=0; i<count; ++i) {
						
						iv2 pos;
						glfwGetMonitorPos(monitors[i], &pos.x,&pos.y);

						auto* mode = glfwGetVideoMode(monitors[i]);

						v2 monitor_center = ((v2)pos +(v2)iv2(mode->width,mode->height)) / 2;
						
						v2 offs = wnd_center -monitor_center;
						flt dist = length(offs);

						if (dist < min_dist) {
							fullscreen_monitor = monitors[i];
							min_dist = dist;

							fullscreen_vidmode = mode;
						}
					}

					assert(fullscreen_monitor);
				}

				iv2 res = fullscreen_resolution;
				if (res.x < 0)
					res = iv2(fullscreen_vidmode->width, fullscreen_vidmode->height);

				glfwSetWindowMonitor(window, fullscreen_monitor, 0, 0, res.x,res.y, GLFW_DONT_CARE);

			} else { // want windowed
				assert(!was_windowed);

				auto r = to_rect(win32_windowplacement.rcNormalPosition);
				auto sz = r.get_size();
				glfwSetWindowMonitor(window, NULL, r.low.x,r.low.y, sz.x,sz.y, GLFW_DONT_CARE);

				set_win32_windowplacement(); // Still refuses to return to maximized mode ??

				//if (win32_windowplacement.showCmd == SW_MAXIMIZE) {
				//	glfwMaximizeWindow(window); // Still refuses to return to maximized mode ????
				//}
			}

			set_vsync(vsync_mode); // seemingly need to reset vsync sometimes when toggling fullscreen mode

			is_fullscreen = want_fullscreen;
		}
		void toggle_fullscreen () {
			set_fullscreen(!is_fullscreen);
		}

		void open (std::string const& title, iv2 default_size=iv2(1280,720), e_vsync_mode vsync_mode=VSYNC_ON) {
			assert(window == nullptr);
			
			this->vsync_mode = vsync_mode;

			glfwSetErrorCallback(glfw_error_proc);

			assert(glfwInit() != 0); // do not support multiple windows at the moment

			bool placement_loaded = load_window_positioning();

			is_fullscreen = false; // always start in windowed mode

			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
			glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);

			bool gl_vaos_required = true; // Having a VAO bound is required in opengl 3.3

			glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, 1);

			s32v2 size = placement_loaded ? to_rect(win32_windowplacement.rcNormalPosition).get_size() : default_size;

			window = glfwCreateWindow(size.x,size.y, title.c_str(), NULL, NULL);

			glfwSetWindowUserPointer(window, this);

			if (placement_loaded) {
				set_win32_windowplacement();
			}

			glfwSetCursorPosCallback(window,		glfw_mouse_pos_event);
			glfwSetMouseButtonCallback(window,		glfw_mouse_button_event);
			glfwSetScrollCallback(window,			glfw_mouse_scroll);
			glfwSetKeyCallback(window,				glfw_key_event);
			glfwSetCharModsCallback(window,			glfw_char_event);

			glfwMakeContextCurrent(window);

			gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

			// setting up some commonly needed opengl state
			if (GLAD_GL_ARB_debug_output) {
				glDebugMessageCallbackARB(ogl_debuproc, 0);
				glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);

				// without GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB ogl_debuproc needs to be thread safe
			}

			GLuint vao; // one global vao for everything

			if (gl_vaos_required) {
				glGenVertexArrays(1, &vao); // if the user (more like the rest of the engine) does not bother to use VAOs we get a blackscreen if we don't bind one global VAO
				glBindVertexArray(vao);
			}

			glEnable(GL_FRAMEBUFFER_SRGB); // we dont want to user to forget this and have a gamma incorrect pipeline

			set_vsync(vsync_mode);
		}
		void close () {
			save_window_positioning();

			glfwDestroyWindow(window);
			window = nullptr;

			glfwTerminate();
		}

		virtual ~Window () {
			if (window)
				close();
		}

		Input& poll_input (bool mouse_cursor_enabled=true) {

			inp.mousecursor.delta_screen = 0;
			inp._mousewheel.delta = 0;
			for (auto& b : inp._buttons) {
				b.went_down = false;
				b.went_up = false;
				b.os_repeat = false;
			}

			inp.events.clear();

			glfwSetInputMode(window, GLFW_CURSOR, mouse_cursor_enabled ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);

			glfwPollEvents();

			glfwGetFramebufferSize(window, &inp.wnd_size_px.x,&inp.wnd_size_px.y);
			set_shared_uniform("common", "common_window_size", (v2)inp.wnd_size_px);

			{
				double x,y;
				glfwGetCursorPos(window, &x, &y);
				inp.mousecursor.pos_screen = v2((flt)x,(flt)y);

				set_shared_uniform("common", "mcursor_pos_window", (v2)inp.wnd_size_px);
			}

			inp.reset_blocked();

			return inp;
		}

		bool wants_to_close () {
			return glfwWindowShouldClose(window) != 0; // also handles ALT+F4
		}

		void set_vsync (e_vsync_mode vsync_mode) {

			int interval = 1;

			if (vsync_mode == VSYNC_ON)
				interval = 1; // -1 swap interval technically requires a extension check which requires wgl loading in addition to gl loading
			else if (vsync_mode == VSYNC_OFF)
				interval = 0;
			else assert(not_implemented);

			glfwSwapInterval(interval);

			this->vsync_mode = vsync_mode;
		}

		void swap_buffers () {
			glfwSwapBuffers(window);
		}

	};

	class Application : public Window {

		Delta_Time_Measure dt_measure;
		bool stop_recursion = false;

		static void run_frame (GLFWwindow* window) {
			auto* app = ((Application*)glfwGetWindowUserPointer(window));

			if (app->stop_recursion) return;
			app->stop_recursion = true;

			app->run_frame();

			app->stop_recursion = false;
		}
		void run_frame () {
			shader_manager.poll_reload_shaders(frame_i);

			auto& inp = poll_input(this->inp.gui_input_enabled);

			if (inp.went_down(GLFW_KEY_F11))
				toggle_fullscreen();

			bool trigger_load = inp.alt_combo('L') || frame_i == 0;
			bool trigger_save = inp.alt_combo('S');
			save = save_file("saves/save.xml", trigger_load, trigger_save);

			static bool imgui_enabled = true;
			if (inp.went_down(GLFW_KEY_F2))
				imgui_enabled = !imgui_enabled;
			if (inp.went_down(GLFW_KEY_F1))
				inp.gui_input_enabled = !inp.gui_input_enabled;

			begin_imgui(&inp, dt, inp.gui_input_enabled, imgui_enabled);

			{
				flt fps = 1.0f / dt;

				static Exp_Moving_Avg dt_avg  (1.0f / 60, 0.2f);
				static Exp_Moving_Avg fps_avg (60, 0.2f);

				if (frame_i > 0) { // fps is inf, dt is 0 on frame 0
					dt_avg.update(dt, dt);
					fps_avg.update(fps, dt);
				}

				ImGui::Text("%8.3f fps  %8.3f ms (%8.3f)", fps_avg.value, dt_avg.value * 1000, dt * 1000);

				{
					static flt values_per_pixel = 0.5f;
					static flt plot_height = 50;
					static bool heartbeat_style = false;

					flt w = ImGui::GetContentRegionAvailWidth();

					int value_count = max(1, (int)round(w * values_per_pixel));

					static std::vector<flt> values (value_count, 0);
					static int cur_value = 0;

					values[cur_value] = dt * 1000;

					cur_value = (cur_value +1) % (int)values.size();

					if ((int)values.size() != value_count) {
						int delta = value_count -(int)values.size();

						if (delta < 0) {
							int values_erasable_at_end = (int)values.size() -cur_value;

							int values_to_erase = -delta;

							int erase_end = min(values_to_erase, values_erasable_at_end);
							int erase_begin = values_to_erase -erase_end;

							values.erase(values.begin() +cur_value, values.begin() +cur_value +erase_end);
							values.erase(values.begin(), values.begin() +erase_begin); // wrap around the erase

							cur_value -= erase_begin;

							cur_value = cur_value % (int)values.size(); // since we erased, cur_value can happen to be at one past the end again

						} else {
							values.insert(values.begin() +cur_value, delta, 0);
						}

					}

					ImGui::PlotLines("##frametime", values.data(), (int)values.size(), heartbeat_style ? 0 : cur_value, "frametime [ms]", 0, 1.0f / 60 * 2.5f * 1000, ImVec2(w, plot_height));
					if (ImGui::BeginPopupContextItem("frametime plot popup")) {
						ImGui::SliderFloat("values_per_pixel", &values_per_pixel, 0, 1.5f);
						ImGui::SliderFloat("plot_height", &plot_height, 0, 200);
						ImGui::Checkbox("heartbeat_style", &heartbeat_style);
						ImGui::EndPopup();
					}
				}

				dt = min(dt, 1.0f / 20); // prevent big timestep when paused or frozen for whatever reason
			}

			//if (	(inp.buttons[GLFW_MOUSE_BUTTON_RIGHT].went_down && !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)) // unfocus imgui windows when right clicking outside of them
			//	|| dsp->frame_i < 3 || !imgui_enabled || !inp.gui_input_enabled ) { // imgui steals focus on the third frame for some reason
			//	ImGui::ClearActiveID();
			//	//ImGui::FocusWindow(NULL);
			//}

			{
				bool vsync = vsync_mode == VSYNC_ON;
				if (imgui::Checkbox("vsync", &vsync))
					set_vsync(vsync ? VSYNC_ON : VSYNC_OFF);
			}
			imgui::SameLine();
			{
				bool fullscreen = is_fullscreen;
				if (imgui::Checkbox("fullscreen", &fullscreen))
					set_fullscreen(fullscreen);
			}
			imgui::SameLine();
			{
				static bool ShowDemoWindow = false;
				imgui::Checkbox("ShowDemoWindow", &ShowDemoWindow);
				if (ShowDemoWindow) {
					imgui::ShowDemoWindow(&ShowDemoWindow);
				}
			}

			imgui::Separator();

			frame();

			draw_to_screen(inp.wnd_size_px);
			end_imgui(inp.wnd_size_px);

			save->end_frame();

			swap_buffers();

			dt = dt_measure.frame();
		}
		static void glfw_resize_or_move_event (GLFWwindow* window) {
			auto* app = ((Application*)glfwGetWindowUserPointer(window));

			run_frame(window);
		}

	public:
		
		virtual ~Application () {}

		flt					dt;
		int					frame_i;

		void run () {

			// This still pauses when you move the window by clicking on the titlebar, but then don't move the mouse
			glfwSetFramebufferSizeCallback(	window, [] (GLFWwindow* window, int x, int y) { glfw_resize_or_move_event(window); } );
			glfwSetWindowPosCallback(		window, [] (GLFWwindow* window, int x, int y) { glfw_resize_or_move_event(window); } );
			glfwSetWindowRefreshCallback(	window, [] (GLFWwindow* window) {				glfw_resize_or_move_event(window); } );

			dt = dt_measure.begin();

			for (frame_i=0;; ++frame_i) {

				run_frame(window);

				if (wants_to_close())
					break;
			}
		}

		virtual void frame () = 0;
	};

}
