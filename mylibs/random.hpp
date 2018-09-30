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

	struct Uniform_Int {
		std::default_random_engine			generator;
		
		Uniform_Int (uint seed) {
			generator.seed(seed);
		}
		Uniform_Int (): Uniform_Int{ (uint)time(NULL) } {}

		int roll (int min, int max) {
			assert(max > min);

			std::uniform_int_distribution<int>	distribution (min, max -1);

			return distribution(generator);
		}
		int roll (int max) {
			return roll(0, max);
		}

		iv2 roll (iv2 min, iv2 max) {
			return iv2(	roll(min.x, max.x),
						roll(min.y, max.y) );
		}
	};

	struct Uniform_Flt {
		std::default_random_engine			generator;

		Uniform_Flt (uint seed) {
			generator.seed(seed);
		}
		Uniform_Flt (): Uniform_Flt{ (uint)time(NULL) } {}

		flt roll (flt min, flt max) {
			assert(max > min);

			std::uniform_real_distribution<flt>	distribution (min, max);

			return distribution(generator);
		}
		flt roll (flt max) {
			return roll(0, max);
		}

		v2 roll (v2 min, v2 max) {
			return v2(	roll(min.x, max.x),
						roll(min.y, max.y) );
		}
		v3 roll (v3 min, v3 max) {
			return v3(	roll(min.x, max.x),
						roll(min.y, max.y),
						roll(min.z, max.z) );
		}
	};
}
