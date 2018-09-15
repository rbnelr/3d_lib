#include "3d_lib/engine.hpp"
#include "3d_lib/common_colors.hpp"
using namespace engine;
using namespace common_colors;

#include "Imgui_File_Browser.hpp"

void frame (Display& dsp, Input& inp, flt dt) {
	
	engine::draw_to_screen(inp.wnd_size_px);
	engine::clear(npp_obsidian::bg_color);

	static imgui_file_browser::Imgui_File_Browser tree ("P:/img_viewer_sample_files/");

	tree.show(inp);
}

int main () {
	engine::run_display(frame, MSVC_PROJECT_NAME);
	return 0;
}
