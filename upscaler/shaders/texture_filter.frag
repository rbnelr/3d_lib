$include "texture_filter_common.frag"

uniform float step_size_px = 1;
uniform float threshold = 0.01;
uniform int gradient_normal_samples = 8;

float luma (vec3 rgb) {
	rgb *= vec3(0.2126, 0.7152, 0.0722);
	return rgb.r + rgb.g + rgb.b;
}

vec2 directional_gradient_detect (vec2 uv, vec3 col, vec2 dir) {
	vec2 textel_size_uv = 1.0 / tex_size_px;

	float a = luma(col);
	float b = luma(texture(tex, uv + dir * textel_size_uv).rgb);
	float diff = b -a;

	//vec3 a = col;
	//vec3 b = texture(tex, uv + dir * textel_size_uv).rgb;
	//float diff = distance(a,b);

	return dir * pow(max(diff -threshold, 0), 2);
}

vec2 calc_gradient_normal (vec2 uv) {

	vec3 col = texture(tex, uv).rgb;

	vec2 total = vec2(0);

	for (int i=0; i<gradient_normal_samples; ++i) {
		vec2 dir = unitvec2(deg(360.0) * float(i) / float(gradient_normal_samples)) * step_size_px;
		total += directional_gradient_detect(uv, col, dir);
	}

	total *= 4 * PI / float(gradient_normal_samples);

	return total;
}

vec3 visualize (vec2 vec) {
	float angle = atan(vec.x +0.001, vec.y) / deg(360);
	return hsl_to_rgb(vec3(angle, 1, 0.5)) * length(vec);
	//return vec3(vec,0);
}

float calc_edge_distance (vec2 gradient) {
	if (length(gradient) == 0) return 999;

	float a = luma(texture(tex, vs_uv).rgb);
	
	int steps = 100;
	float step_size = 0.01;
	
	float dist = 0;
	
	int i;
	for (i=0; i<steps; ++i) {
		vec2 dir = -gradient * (dist +step_size / 2);
	
		float b = luma(texture(tex, vs_uv +dir * step_size_px).rgb);
	
		if (b < a)
			break;
	
		dist += step_size;
	}
	
	return float(i) / 100.0;
}


vec4 filter () {
	vec4 rgba = texture(tex, vs_uv).rgba;
	vec3 col = rgba.rgb;
	float alpha = rgba.a;
	
	DEBUG(vec4(1,0,0,1));

	if (dbg_right())
		return rgba;

	vec2 gradient = calc_gradient_normal(vs_uv);
	return vec4(visualize(gradient), 1);

	float dist = calc_edge_distance(gradient);

	return vec4(vec3(dist), alpha);
}
