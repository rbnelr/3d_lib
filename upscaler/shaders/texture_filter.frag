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
	float angle = atan(vec.y +0.001, vec.x) / deg(360);
	return hsl_to_rgb(vec3(angle, 1, 0.5)) * length(vec);
	//return vec3(vec,0);
}

float calc_edge_distance (vec2 gradient) {
	if (length(gradient) < 0.001) return 1;

	gradient = normalize(gradient);

	vec2 textel_size_uv = 1.0 / tex_size_px;

	int steps = 32;
	float step_size = 1.5 / float(steps);
	
	float dist = 0;
	
	int i;
	for (i=0; i<steps; ++i) {
		vec2 dir = -gradient * (dist +textel_size_uv / 2);
	
		vec2 gradient_b = calc_gradient_normal(vs_uv +dir * textel_size_uv);
	
		if (dot(gradient, gradient_b) < 0.001)
			break;
	
		dist += step_size;
	}
	
	return float(i) / float(steps);
}


vec4 filter () {
	vec4 rgba = texture(tex, vs_uv).rgba;
	vec3 col = rgba.rgb;
	float alpha = rgba.a;
	
	vec2 gradient = calc_gradient_normal(vs_uv);
	
	//if (dbg_right() && dbg_up())
	//	DEBUG(visualize(gradient));

	float dist = calc_edge_distance(gradient);
	
	//if (dbg_left() && dbg_up())
	if (dbg_left())
		DEBUG(rgba);

	if (length(gradient) >= 0.001)
		col = dist > 0.5 ? vec3(1,1,1) : vec3(0,0,0);

	return vec4(col, alpha);
	//return vec4(vec3(dist), alpha);
}
