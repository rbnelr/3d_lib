#pragma once

#include "basic_typedefs.hpp"
#include "vector.hpp"
#include "float_precision.hpp"

#include <random>

#include "time.h"
#include "assert.h"

namespace random {
	using namespace basic_typedefs;
	using namespace vector;
	using namespace float_precision;

	std::default_random_engine global_generator = std::default_random_engine ( (uint)time(NULL) );

	int uniform (std::default_random_engine& generator, int min, int max) {
		assert(max > min);

		std::uniform_int_distribution<int>	distribution (min, max -1);

		return distribution(generator);
	}

	int uniform (int min, int max) {
		return uniform(global_generator, min, max);
	}
	int uniform (int max) {
		return uniform(0, max);
	}

	iv2 uniform (iv2 min, iv2 max) {
		return iv2(	uniform(min.x, max.x),
					uniform(min.y, max.y) );
	}

	flt uniform (std::default_random_engine& generator, flt min, flt max) {
		assert(max > min);

		std::uniform_real_distribution<flt>	distribution (min, max);

		return distribution(generator);
	}

	flt uniform (flt min, flt max) {
		return uniform(global_generator, min, max);
	}
	flt uniform (flt max) {
		return uniform(0.0f, max);
	}

	v2 uniform (v2 min, v2 max) {
		return v2(	uniform(min.x, max.x),
					uniform(min.y, max.y) );
	}
	v3 uniform (v3 min, v3 max) {
		return v3(	uniform(min.x, max.x),
					uniform(min.y, max.y),
					uniform(min.z, max.z) );
	}
}
