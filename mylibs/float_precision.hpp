#pragma once

#include "basic_typedefs.hpp"
#include "vector.hpp"

// Use 32bit float as float type by default, if everything goes well you could switch to doubles by simply changing the typedef here, but i never tested this, NOTE that OpenGL does not support doubles
namespace float_precision {
	using namespace basic_typedefs;
	using namespace vector;

	typedef f32 flt;

	typedef fv2 v2;
	typedef fv3 v3;
	typedef fv4 v4;

	typedef s32v2 iv2;
	typedef s32v3 iv3;
	typedef s32v4 iv4;

	typedef fm2 m2;
	typedef fm3 m3;
	typedef fm4 m4;
	typedef fhm hm;

	typedef fquat quat;
}
