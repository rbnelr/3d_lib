
// common uniforms
uniform	vec2	common_window_size;
uniform	vec2	common_mcursor_pos_window;
uniform	vec2	common_viewport_offset;
uniform	vec2	common_viewport_size;

vec2 mcursor () { // [0,1] in viewport
	return (common_mcursor_pos_window -common_viewport_offset) / common_viewport_size;
}

// srgb conversions
vec3 to_srgb (vec3 linear) {
	bvec3 cutoff = lessThanEqual(linear, vec3(0.00313066844250063));
	vec3 higher = vec3(1.055) * pow(linear, vec3(1.0/2.4)) -vec3(0.055);
	vec3 lower = linear * vec3(12.92);

	return mix(higher, lower, cutoff);
}
vec3 to_linear (vec3 srgb) {
	bvec3 cutoff = lessThanEqual(srgb, vec3(0.0404482362771082));
	vec3 higher = pow((srgb +vec3(0.055)) / vec3(1.055), vec3(2.4));
	vec3 lower = srgb / vec3(12.92);

	return mix(higher, lower, cutoff);
}
vec4 to_srgb (vec4 linear) {	return vec4(to_srgb(linear.rgb), linear.a); }
vec4 to_linear (vec4 srgba) {	return vec4(to_linear(srgba.rgb), srgba.a); }

vec3 srgb (int r, int g, int b) {			return to_linear(vec3(ivec3(r,g,b)) / 255); }
vec4 srgb (int r, int g, int b, int a) {	return to_linear(vec4(ivec4(r,g,b,a)) / 255); }

vec3 hsl_to_rgb (vec3 hsl) { // hue is periodic since it represents the angle on the color wheel, so it can be out of the range [0,1]
	float hue = hsl.x;
	float sat = hsl.y;
	float lht = hsl.z;

	float hue6 = mod(hue, 1.0) * 6.0;

	float c = sat*(1.0 -abs(2.0*lht -1.0));
	float x = c * (1.0 -abs(mod(hue6, 2.0) -1.0));
	float m = lht -(c/2.0);

	vec3 rgb;
	if (		hue6 < 1.0 )	rgb = vec3(c,x,0);
	else if (	hue6 < 2.0 )	rgb = vec3(x,c,0);
	else if (	hue6 < 3.0 )	rgb = vec3(0,c,x);
	else if (	hue6 < 4.0 )	rgb = vec3(0,x,c);
	else if (	hue6 < 5.0 )	rgb = vec3(x,0,c);
	else						rgb = vec3(c,0,x);
	rgb += vec3(m);

	return rgb;
}

float map (float x, float a, float b) {
	return (x -a) / (b -a);
}
float map (float x, float in_a, float in_b, float out_a, float out_b) {
	return mix(out_a, out_b, map(x, in_a, in_b));
}

#define PI 3.1415926535897932384626433832795

#define DEG_TO_RAD	0.01745329251994329576923690768489	// 180/PI
#define RAD_TO_DEG	57.295779513082320876798154814105	// PI/180

float to_rad (float ang) {	return ang * DEG_TO_RAD; }
float deg (float ang) {		return ang * DEG_TO_RAD; }
float to_deg (float ang) {	return ang * RAD_TO_DEG; }

vec2 unitvec2 (float rad) {
	return vec2(cos(rad), sin(rad));
}
