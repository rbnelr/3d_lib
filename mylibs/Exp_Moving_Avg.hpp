#pragma once

#include "basic_typedefs.hpp"
#include "float_precision.hpp"

namespace exp_moving_avg {
	using namespace float_precision;
	struct Exp_Moving_Avg {
		// https://stackoverflow.com/questions/1023860/exponential-moving-average-sampled-at-varying-times

		flt	value;
		flt	time; // time constant to tune to smooth value with

		Exp_Moving_Avg (flt inital_value, flt time) {
			this->value = inital_value;
			this->time = time;
		}

		flt update (flt cur_value, flt dt) { // dont pass NaN or Inf into this, since that will permanently destroy lock the value
			flt alpha = 1 -exp(-dt / time);

			value += (cur_value -value) * alpha;
			return value;
		}
	};
}
using exp_moving_avg::Exp_Moving_Avg;
