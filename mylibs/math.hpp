#pragma once
#include <cmath>

#include "basic_typedefs.hpp"
#include "compiler_specific.hpp" // for INF & NAN

#if 0 // debug
#define constexpr 
#define ALWAYSINLINE inline
#define INL inline
#else
#define ALWAYSINLINE FORCEINLINE // to prevent tons of small functions in debug mode
#define INL inline
#endif

namespace math {
	using namespace basic_typedefs;

	using std::abs;
	using std::sin;
	using std::cos;
	using std::floor;
	using std::ceil;
	using std::pow;
	using std::fmod;

	// windows.h defines min and max by default
	#undef min
	#undef max

	#define INFd		F64_INF
	#define INF			F32_INF

	#define QNANd		F64_QNAN
	#define QNAN		F32_QNAN

	#define PId			3.1415926535897932384626433832795
	#define PI			3.1415926535897932384626433832795f

	#define TAUd		6.283185307179586476925286766559
	#define TAU			6.283185307179586476925286766559f

	#define SQRT_2d		1.4142135623730950488016887242097
	#define SQRT_2		1.4142135623730950488016887242097f

	#define EULERd		2.7182818284590452353602874713527
	#define EULER		2.7182818284590452353602874713527f

	#define DEG_TO_RADd	0.01745329251994329576923690768489	// 180/PI
	#define DEG_TO_RAD	0.01745329251994329576923690768489f

	#define RAD_TO_DEGd	57.295779513082320876798154814105	// PI/180
	#define RAD_TO_DEG	57.295779513082320876798154814105f

	INL constexpr f32 to_rad (f32 deg) {	return deg * DEG_TO_RAD; }
	INL constexpr f32 deg (f32 deg) {		return deg * DEG_TO_RAD; } // degress "literal", converts degrees to radiants
	INL constexpr f32 to_deg (f32 rad) {	return rad * RAD_TO_DEG; }

	INL constexpr f64 to_rad (f64 deg) {	return deg * DEG_TO_RADd; }
	INL constexpr f64 degd (f64 deg) {		return deg * DEG_TO_RADd; } // degress "literal", converts degrees to radiants
	INL constexpr f64 to_deg (f64 rad) {	return rad * RAD_TO_DEGd; }

	#define RAD_360	deg(360)
	#define RAD_180	deg(180)
	#define RAD_90	deg(90)

	// length and normalization for "1 dimensional vectors" ie. scalars
	INL f32 length (f32 x) {	return abs(x); }
	INL s32 length (s32 x) {	return abs(x); }

	INL f32 normalize (f32 x) {	return x / length(x); }
	INL s32 normalize (s32 x) {	return x / length(x); }

	template <typename T>
	INL T normalize_or_zero (T v) { // TODO: epsilon?
		auto len = length(v);
		if (len != 0) {
			v /= len;
		}
		return v;
	}

	INL constexpr f32 select (bool c, f32 l, f32 r) {	return c ? l : r; }
	INL constexpr s32 select (bool c, s32 l, s32 r) {	return c ? l : r; }

	INL constexpr s32 min (s32 l, s32 r) { return l <= r ? l : r; }
	INL constexpr s32 max (s32 l, s32 r) { return l >= r ? l : r; }
	INL constexpr u32 min (u32 l, u32 r) { return l <= r ? l : r; }
	INL constexpr u32 max (u32 l, u32 r) { return l >= r ? l : r; }
	INL constexpr s64 min (s64 l, s64 r) { return l <= r ? l : r; }
	INL constexpr s64 max (s64 l, s64 r) { return l >= r ? l : r; }
	INL constexpr u64 min (u64 l, u64 r) { return l <= r ? l : r; }
	INL constexpr u64 max (u64 l, u64 r) { return l >= r ? l : r; }

	INL constexpr f32 min (f32 l, f32 r) { return l <= r ? l : r; }
	INL constexpr f32 max (f32 l, f32 r) { return l >= r ? l : r; }
	INL constexpr f64 min (f64 l, f64 r) { return l <= r ? l : r; }
	INL constexpr f64 max (f64 l, f64 r) { return l >= r ? l : r; }

	INL constexpr s32 clamp (s32 val, s32 l, s32 h) {	return min( max(val,l), h ); }
	INL constexpr u32 clamp (u32 val, u32 l, u32 h) {	return min( max(val,l), h ); }
	INL constexpr s64 clamp (s64 val, s64 l, s64 h) {	return min( max(val,l), h ); }
	INL constexpr u64 clamp (u64 val, u64 l, u64 h) {	return min( max(val,l), h ); }

	INL constexpr f32 clamp (f32 val, f32 l, f32 h) {	return min( max(val,l), h ); }
	INL constexpr f64 clamp (f64 val, f64 l, f64 h) {	return min( max(val,l), h ); }

	INL constexpr f32 lerp (f32 a, f32 b, f32 t) {		return a*(1.0f -t) +b*t; }

	INL constexpr f32 map (f32 x, f32 in_a, f32 in_b) {	return (x -in_a)/(in_b -in_a); }
	INL constexpr f32 map (f32 x, f32 in_a, f32 in_b, f32 out_a, f32 out_b) {
															return lerp(out_a, out_b, map(x, in_a, in_b)); }

	template <typename T> INL T mymod (T x, T range) {
		#if 1
		T remainder = fmod(x, range);
		if (range > 0) {
			if (remainder < 0) remainder += range;
		} else {
			if (remainder > 0) remainder -= range;
		}
		return remainder;
		#endif
	}

	template <typename T> INL T mod_range (T x, T a, T b) {
		x -= a;
		T range = b -a;

		T modulo = mymod(x, range);

		return modulo + a;
	}
	template <typename T> INL T mod_range (T x, T a, T b, T* quotient) {
		x -= a;
		T range = b -a;

		T modulo = mymod(x, range);
		*quotient = floor(x / range);

		return modulo + a;
	}

	f32 smoothstep (f32 x) {
		f32 t = clamp(x, 0.0f, 1.0f);
		return t * t * (3.0f - 2.0f * t);
	}

	// Getting sin and cos of same angle can be optimized to be almost twice as fast (sin cos calculation are almost identical i've heard)
	struct Sin_Cos {	f32 s, c; };
	struct Sin_CosD {	f64 s, c; };

	INL Sin_Cos		sin_cos (f32 ang) { return { sin(ang), cos(ang) }; }
	INL Sin_CosD	sin_cos (f64 ang) { return { sin(ang), cos(ang) }; }

	template <typename T> INL bool equal_epsilon (T l, T r, T epsilon) {
		return abs(l -r) <= epsilon;
	}

	#if 1
	union FI32_u {
		f32		f;
		u32		i;
		INL FI32_u(f32 f): f{f} {}
		INL FI32_u(u32 i): i{i} {}
	};
	union FI64_u {
		f64		f;
		u64		i;
		INL FI64_u(f64 f): f{f} {}
		INL FI64_u(u64 i): i{i} {}
	};

	INL f32 int_bits_as_flt (u32 i) {
		return FI32_u(i).f;
	}
	INL f64 int_bits_as_flt (u64 i) {
		return FI64_u(i).f;
	}
	INL u32 flt_bits_as_int (f32 f) {
		return FI32_u(f).i;
	}
	INL u64 flt_bits_as_int (f64 f) {
		return FI64_u(f).i;
	}
	#endif
}
