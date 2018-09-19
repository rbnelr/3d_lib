#pragma once

#include "basic_typedefs.hpp"
#include "float_precision.hpp"
#include "windows.h"
#include "assert.h"

namespace QPC {
	using namespace basic_typedefs;
	using namespace float_precision;

	u64 get_freq () {
		u64 val;
		assert(QueryPerformanceFrequency((LARGE_INTEGER*)&val));
		return val;
	}
	u64 get_timestemp () {
		u64 val;
		assert(QueryPerformanceCounter((LARGE_INTEGER*)&val));
		return val;
	}

	u64 freq = get_freq();

	struct Timer {

		u64 begin;
		flt seconds;

		void start () {
			begin = get_timestemp();
		}

		flt end () {
			u64 now = get_timestemp();

			u64 delta = now -begin;

			seconds = (flt)delta / (flt)freq;
			return seconds;
		}

	};

	struct Delta_Time_Measure {

		u64 prev_frame_end;

		flt begin () { // call one before frame loop
			prev_frame_end = get_timestemp();
			return 0; // zero dt on first frame, timestep calc should be able to handle this
		}

		flt frame () { // call after first frame
			u64 now = get_timestemp();

			u64 dt_ticks = now -prev_frame_end;

			flt dt = (flt)dt_ticks / (flt)freq;

			prev_frame_end = now;
			return dt;
		}

	};
}
using QPC::Timer;
using QPC::Delta_Time_Measure;
