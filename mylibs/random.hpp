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

	struct Generator {
		std::default_random_engine generator;

		Generator (): generator{ (uint)time(NULL) } {} // random seed
		Generator (uint seed): generator{seed} {} // seed with value
		Generator (int seed): Generator((uint)seed) {} // seed with value
	};

	Generator global_generator = Generator();

	// int
	int uniform (Generator& generator, int min, int max) {
		assert(max > min);

		std::uniform_int_distribution<int>	distribution (min, max -1);

		return distribution(generator.generator);
	}
	int uniform (int min, int max) {						return uniform(global_generator, min,max); }

	int uniform (Generator& generator, int max) {			return uniform(generator, 0, max); }
	int uniform (int max) {									return uniform(global_generator, max); }

	iv2 uniform (Generator& generator, iv2 min, iv2 max) {	return iv2(	uniform(generator, min.x, max.x), uniform(generator, min.y, max.y) ); }
	iv2 uniform (iv2 min, iv2 max) {						return uniform(global_generator, min,max); }

	// float
	flt uniform (Generator& generator, flt min, flt max) {
		assert(max > min);

		std::uniform_real_distribution<flt>	distribution (min, max);

		return distribution(generator.generator);
	}
	flt uniform (flt min, flt max) {						return uniform(global_generator, min,max); }

	flt uniform (Generator& generator, flt max) {			return uniform(generator, 0.0f, max); }
	flt uniform (flt max) {									return uniform(global_generator, max); }

	v2 uniform (Generator& generator, v2 min, v2 max) {		return v2(	uniform(generator, min.x, max.x), uniform(generator, min.y, max.y) ); }
	v2 uniform (v2 min, v2 max) {							return uniform(global_generator, min,max); }
	
}
