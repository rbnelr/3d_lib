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

		Generator (): generator{ get_rand_seed() } {} // random seed
		Generator (uint seed): generator{seed} {} // seed with value
		Generator (int seed): Generator((uint)seed) {} // seed with value

		static int get_rand_seed () {
			//return (int)time(NULL); // only ticks every second!!

			LARGE_INTEGER li;
			QueryPerformanceCounter(&li);
			return (int)li.QuadPart;
		}
	};

	Generator global_generator = Generator();

	// chance
	int chance (Generator& generator, flt prob=0.5f) {
		std::bernoulli_distribution	distribution (prob);

		return distribution(generator.generator);
	}
	bool chance (flt prob=0.5f) {							return chance(global_generator, prob) ? 1 : 0; }

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
	
	//
	flt normal (Generator& generator, flt stddev, flt mean=0) {
		std::normal_distribution<flt>	distribution (mean, stddev);

		return distribution(generator.generator);
	}
	flt normal (flt stddev, flt mean=0) {					return normal(global_generator, stddev,mean); }

	v2 normal (Generator& generator, v2 stddev, v2 mean=0) {	return v2( normal(generator, stddev.x,mean.x), normal(generator, stddev.y,mean.y) ); }
	v2 normal (v2 stddev, v2 mean=0) {							return normal(global_generator, stddev,mean); }

}
