#pragma once

#include "basic_typedefs.hpp"
#include "vector.hpp"
#include "float_precision.hpp"

#include "stdlib.h"
#include "time.h"

namespace random {
	using namespace basic_typedefs;
	using namespace vector;
	using namespace float_precision;

	uint seed_random () {
		auto seed = (uint)time(NULL);
		srand(seed);
		return seed;
	}
	void seed (uint val) {
		srand(val);
	}

	flt rand_float (flt lower=0, flt upper=1) {
		return ((flt)rand() / (flt)RAND_MAX) * (upper -lower) +lower;
	}

	v2 rand_v2 (v2 lower, v2 upper) {
		return v2(rand_float(lower.x,upper.x), rand_float(lower.y,upper.y));
	}
}
