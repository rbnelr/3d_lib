#pragma once

// needs fv3 fv4 typedefs

#include "vector.hpp"

namespace colors {
	using namespace basic_typedefs;
	using namespace vector;
	
	template <typename T> inline T to_linear (T srgb) {
		return select(srgb <= T(0.0404482362771082f),
					  srgb * (T(1)/T(12.92f)),
					  pow( (srgb +T(0.055f)) * T(1/1.055f), T(2.4f) )
		);
	}
	template <typename T> inline T to_srgb (T linear) {
		return select(linear <= T(0.00313066844250063f),
					  linear * T(12.92f),
					  ( T(1.055f) * pow(linear, T(1/2.4f)) ) -T(0.055f)
		);
	}

	typedef fv3		lrgb;
	typedef fv4		lrgba;

	struct srgb8 {
		u8v3	v;

		constexpr explicit srgb8 (u8v3 v): v{v} {}

		srgb8 () {}
		constexpr srgb8 (u8 all): v{all} {}
		constexpr explicit srgb8 (u8 r, u8 g, u8 b): v{r,g,b} {}

		static srgb8 from_linear (lrgb l) {
			return srgb8( (u8v3)(to_srgb(l) * 255 +0.5f) );
		}
		lrgb to_lrgb () const {
			return to_linear((fv3)v / 255);
		}
	};
	struct srgba8 {
		u8v4	v;

		constexpr explicit srgba8 (u8v4 v): v{v} {}

		srgba8 () {}
		constexpr srgba8 (u8 all): v{all, 255} {}
		constexpr srgba8 (srgb8 rgb): v{rgb.v, 255} {}
		constexpr explicit srgba8 (u8 r, u8 g, u8 b): v{r,g,b,255} {}
		constexpr explicit srgba8 (u8 r, u8 g, u8 b, u8 a): v{r,g,b,a} {}

		static srgba8 from_linear (lrgba l) {
			return srgba8( (u8v4)(fv4(to_srgb(l.xyz()), l.w) * 255 +0.5f) );
		}
		lrgba to_lrgb () const {
			fv4 tmp = (fv4)v / 255;
			return fv4(to_linear(tmp.xyz()), tmp.z);
		}
	};

	// TODO: think about srgb vs lrgb here
	inline fv3 _hsl_to_rgb (fv3 hsl) { // hue is periodic since it represents the angle on the color wheel, so it can be out of the range [0,1]
		f32 hue = hsl.x;
		f32 sat = hsl.y;
		f32 lht = hsl.z;

		f32 hue6 = mymod(hue, 1.0f) * 6.0f;

		f32 c = sat*(1.0f -abs(2.0f*lht -1.0f));
		f32 x = c * (1.0f -abs(fmodf(hue6, 2.0f) -1.0f));
		f32 m = lht -(c/2.0f);

		fv3 srgb;
		if (		hue6 < 1.0f )	srgb = fv3(c,x,0);
		else if (	hue6 < 2.0f )	srgb = fv3(x,c,0);
		else if (	hue6 < 3.0f )	srgb = fv3(0,c,x);
		else if (	hue6 < 4.0f )	srgb = fv3(0,x,c);
		else if (	hue6 < 5.0f )	srgb = fv3(x,0,c);
		else						srgb = fv3(c,0,x);
		srgb += m;

		return srgb;
	}
}
