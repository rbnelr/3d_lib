
#include "3d_lib/engine.hpp"
#include "3d_lib/camera2d.hpp"
#include "mylibs/random.hpp"
using namespace engine;

struct Particle {
	v2	pos = 0;
	v2	vel = 0;
	//v2	size = 1;

	static const Vertex_Layout layout;
};
const Vertex_Layout Particle::layout = { (int)sizeof(Particle), {
	{ "particle_pos",			FV2,	(int)offsetof(Particle, pos) },
	{ "particle_vel",			FV2,	(int)offsetof(Particle, vel) },
	//{ "particle_size",			FV2,	(int)offsetof(Particle, size) },
}};

struct Particles {
	
	int count = 2000;
	std::vector<Particle> particles;

	flt		dt_multiplier = 1;
	bool	paused = false;

	flt		size = 0.5f;
	lrgba	col = srgb8(255,4,4).to_lrgba();

	Gpu_Mesh instanced_mesh;
	Instanced_Draw vbo;

	Particle spawn_particle (v2 world_size) {
		Particle p;
		p.pos = random::uniform(-world_size/2, +world_size/2);
		p.vel = random::normal(v2(1)) * size * 10;
		return p;
	}
	void gen_vbo () {
		instanced_mesh = engine::gen_rect<Vertex_Draw_Rect>([] (v2 p, v2 uv) { return Vertex_Draw_Rect{p}; }).upload();
		
		vbo.instanced_mesh = &instanced_mesh;
		vbo.instance_data = Gpu_Mesh::generate<Particle>();
	}

	bool predict_collision (v2 a_pos, v2 a_vel, flt a_radius, v2 b_pos, v2 b_vel, flt b_radius, flt* time_to_collision) {
		// assume linear movement (no acceleration)
		// collision critera: distance(a.pos,b.pos) <= (a.radius + b.radius)
		// dist_a_b(t) should be a quadratic function, solve for y=(a.radius + b.radius) intersection, if exists and t<=dt then we have a collision

		v2 pos_rel = b_pos - a_pos;
		v2 vel_rel = b_vel - a_vel;

		flt coll_dist = a_radius + b_radius;

		flt speed_rel = length(vel_rel);

		if (speed_rel == 0) { // no movement
			*time_to_collision = 0;
			return length_sqr(pos_rel) <= coll_dist*coll_dist; // either collision already or never
		}

		// line from rel pos and rel vel
		v2 dir = vel_rel / speed_rel;
		v2 normal = rotate2_90(dir);

		flt closest_dist = abs(dot(normal, pos_rel)); // distance between line and origin (particle a)
		if (closest_dist > coll_dist)
			return false; // no collision

		flt dist_to_closest = dot(dir, -pos_rel); // dist along line to closest point

		flt half_secant = sqrt(coll_dist*coll_dist -closest_dist*closest_dist); // a^2 = c^2 - b^2

		flt dist_to_coll = dist_to_closest -half_secant;

		*time_to_collision = dist_to_coll / speed_rel; // could be negative
		return dist_to_closest >= 0; // if the particles are moving away from each other we dont report an collision, this should prevent particles from getting stuck but also them moving into each other
	}

	void update (v2 world_size, Input& inp, flt dt) {
		if (!vbo.instanced_mesh)
			gen_vbo();
		
		if (imgui::Button("Respawn Particles") || inp.went_down('R'))
			particles.clear();
		
		imgui::DragInt("count", &count, 1.0f/10, 0,INT_MAX);
		imgui::DragFloat("size", &size, 1.0f/100);
		imgui::ColorEdit_srgb("col", &col);

		imgui::Checkbox("paused", &paused);
		if (inp.went_down('P'))	paused = !paused;
		imgui::DragFloat("dt_multiplier", &dt_multiplier, 1.0f/100);
		
		int old_count = (int)particles.size();
		particles.resize(MAX(count, 0));

		for (int i=old_count; i<count; ++i)
			particles[i] = spawn_particle(world_size);

		flt use_dt = paused ? 0 : dt_multiplier * dt;

		for (auto& p : particles) {

			flt earliest_coll_t = INF;
			Particle* other_p = nullptr;

			for (auto& other : particles) {
				flt coll_t;
				if (&p != &other && predict_collision(p.pos, p.vel, size/2, other.pos, other.vel, size/2, &coll_t)) {
					earliest_coll_t = MIN(earliest_coll_t, coll_t);
					other_p = &other;
				}
			}

			if (earliest_coll_t >= 0 && earliest_coll_t < use_dt) {
				// update both colliding particles

				v2 avg_vel = (p.vel + other_p->vel) / 2;

				p.pos += p.vel * earliest_coll_t;
				p.vel = avg_vel;

				other_p->pos += other_p->vel * earliest_coll_t;
				other_p->vel = avg_vel;
				continue;
			}

			p.pos += p.vel * use_dt;
		}
	}

	void draw (Camera2D& cam) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);
		glDisable(GL_CULL_FACE);
		glDisable(GL_SCISSOR_TEST);

		inline_shader("draw_particles.vert", R"_SHAD(
			$include "common.vert"

			in		vec2	pos_model;

			out		vec2	pos_rect;

			// particle data
			in		vec2	particle_pos;
			in		vec2	particle_vel;
			uniform	float	particle_size;
			uniform	vec4	particle_col;

			out		vec2	pos;
			out		vec2	vel;
			out		float	size;
			out		vec4	col;

			uniform	mat4	view_world_to_cam;
			uniform	mat4	view_cam_to_clip;
			
			uniform	float	px_size_world;

			void vert () {
				pos_rect = pos_model * (particle_size + px_size_world); // pos in rect scaled to world space, 0.5 px extra border for AA
				
				gl_Position = view_cam_to_clip * view_world_to_cam * vec4(particle_pos + pos_rect, 0,1);

				pos		= particle_pos;
				vel		= particle_vel;
				size	= particle_size;
				col		= particle_col;
			}
		)_SHAD");
		inline_shader("draw_particles.frag", R"_SHAD(
			$include "common.frag"

			in		vec2	pos_rect;

			// particle data
			in		vec2	pos;
			in		vec2	vel;
			in		float	size;
			in		vec4	col;
			
			uniform	float	px_size_world;

			vec4 frag () {
				float r = length(pos_rect);

				float alpha = clamp(map(r, size/2 + px_size_world/2, size/2 - px_size_world/2), 0,1);
				
				//clamp(map(abs(r -(size/2 -px_size_world/2)), px_size_world, -px_size_world), 0,1)  ring
				
				if (alpha <= 0)
					discard;

				alpha *= clamp(size / px_size_world, 0,1); // Particles smaller than 1 pixel are dimmer, This calculation is no completely correct

				return vec4(col.rgb, col.a * alpha);
			}
		)_SHAD");

		auto* s = use_shader("draw_particles");
		if (s) {
			vbo.instance_data.reupload(&particles[0], (GLuint)particles.size(), nullptr,0, &Particle::layout);

			set_uniform(s, "particle_size", size);
			set_uniform(s, "particle_col", col);

			set_uniform(s, "px_size_world", cam.size_world.x / (flt)cam.get_subrect().size_px.x);

			vbo.draw(*s);
		}
	}
};

class App : public Application {
	
	v2 world_size = v2(140, 100);

	Camera2D cam = Camera2D(0, world_size);

	lrgb bg_col = srgb8(32,32,32).to_lrgb();

	Particles particles;

	void frame () {
		
		cam.update(inp, dt);
		cam.draw_to();

		imgui::Separator();

		imgui::ColorEdit_srgb("bg_col", &bg_col);

		particles.update(world_size, inp,dt);

		//
		engine::draw_to_screen(inp.wnd_size_px);
		
		clear(0);
		draw_rect(0, world_size, lrgba(bg_col, 1));

		particles.draw(cam);
	}
} app;

int main () {
	app.open(MSVC_PROJECT_NAME);
	app.run();
	return 0;
}
