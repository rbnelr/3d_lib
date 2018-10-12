#pragma once

#include "engine_include.hpp"
#include "gl_shader.hpp"
#include "deps/glad/glad.h"

namespace engine {
namespace vertex_layout {
//

enum type_e { // Types for shader attribs
	FLT, FV2, FV3, FV4,
	INT_, IV2, IV3, IV4,
	U8V4_AS_FV4,

	RGBA8 = U8V4_AS_FV4,
};

struct Vertex_Attribute {
	std::string	name;
	type_e		type;
	int			offset; // offsetof(Vertex, attribute_memeber);
};
struct Shader_Vertex_Attribute {
	std::string	name;
	type_e		type;
	GLint		location;
};

class VAO {
	MOVE_ONLY_CLASS(VAO)

	GLuint handle = 0;
	static GLuint prev_bound_handle; // inited to 0
	
public:
	~VAO () {
		if (handle) // maybe this helps to optimize out destructing of unalloced vaos
			glDeleteVertexArrays(1, &handle); // would be ok to delete unalloced vao (handle = 0)
	}
	
	void bind () const {
		static GLuint prev = 0;

		if (handle != prev) // doing a glBindVertexArray anywhere outside this function will break (potentially crash) this optimization
			glBindVertexArray(handle);

		prev = handle;
	}

	static VAO generate () {
		VAO vao;
		glGenVertexArrays(1, &vao.handle);
		return vao;
	}
};
inline void swap (VAO& l, VAO& r) {
	::std::swap(l.handle, r.handle);
}

class VBO {
	MOVE_ONLY_CLASS(VBO)

	GLuint handle = 0;

public:
	GLuint get_handle () const { return handle; }
	~VBO () {
		if (handle) // maybe this helps to optimize out destructing of unalloced vbos
			glDeleteBuffers(1, &handle); // would be ok to delete unalloced vbo (handle = 0)
	}

	void bind () const {
		static GLuint prev = 0;

		if (handle != prev) // doing a glBindBuffer anywhere outside this function will break (potentially crash) this optimization
			glBindBuffer(GL_ARRAY_BUFFER, handle);

		prev = handle;
	}

	static VBO generate () {
		VBO vbo;
		glGenBuffers(1, &vbo.handle);
		return vbo;
	}
	static VBO gen_and_upload (void const* vertex_data, GLsizeiptr total_size) {
		auto vbo = generate();
		vbo.bind();
		glBufferData(GL_ARRAY_BUFFER, total_size, vertex_data, GL_STATIC_DRAW);
		return vbo;
	}
	void reupload (void const* vertex_data, GLsizeiptr total_size) {
		bind();
		glBufferData(GL_ARRAY_BUFFER, total_size, NULL, GL_DYNAMIC_DRAW); // Buffer orphan on reupload
		if (total_size > 0) // leave buffer as null if there is no data
			glBufferData(GL_ARRAY_BUFFER, total_size, vertex_data, GL_DYNAMIC_DRAW);
	}
};
inline void swap (VBO& l, VBO& r) {
	::std::swap(l.handle, r.handle);
}

class EBO {
	MOVE_ONLY_CLASS(EBO)

	GLuint handle = 0;
	static GLuint prev_bound_handle; // inited to 0
	
public:
	GLuint get_handle () const { return handle; }
	~EBO () {
		if (handle) // maybe this helps to optimize out destructing of unalloced vbos
			glDeleteBuffers(1, &handle); // would be ok to delete unalloced vbo (handle = 0)
	}

	void bind () const {
		static GLuint prev = 0;

		if (handle != prev) // doing a glBindBuffer anywhere outside this function will break (potentially crash) this optimization
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, handle);

		prev = handle;
	}

	static EBO generate () {
		EBO ebo;
		glGenBuffers(1, &ebo.handle);
		return ebo;
	}
	static EBO gen_and_upload (void const* index_data, GLsizeiptr total_size) {
		auto ebo = generate();
		ebo.bind();
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, total_size, index_data, GL_STATIC_DRAW);
		return ebo;
	}
	void reupload (void const* index_data, GLsizeiptr total_size) {
		bind();
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, total_size, NULL, GL_DYNAMIC_DRAW); // Buffer orphan on reupload
		if (total_size > 0) // leave buffer as null if there is no data
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, total_size, index_data, GL_DYNAMIC_DRAW);
	}
};
inline void swap (EBO& l, EBO& r) {
	::std::swap(l.handle, r.handle);
}

// Not using VAOs for now, since it seems like they either require you to come up with meaningful indecies for attributes (0 is always position, index 7 is always vertex color) which i have a hard time coming up with an automated way of doing
//  or have one VAO for each combination of Vertex_Layout and Shader (in which case is there even a benefit to using them?)

struct Vertex_Layout { // assume interleaved (array of vertex structs)
	int		vertex_size; // sizeof(Vertex) == stride

	void setup_attrib (GLint loc, Vertex_Attribute const& attr, GLsizei stride) const {

		glEnableVertexAttribArray(loc);

		switch (attr.type) {
			case FLT:			glVertexAttribPointer(loc, 1, GL_FLOAT, GL_FALSE,			stride, (void*)(uptr)attr.offset);	break;
			case FV2:			glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE,			stride, (void*)(uptr)attr.offset);	break;
			case FV3:			glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE,			stride, (void*)(uptr)attr.offset);	break;
			case FV4:			glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE,			stride, (void*)(uptr)attr.offset);	break;

			case INT_:			glVertexAttribIPointer(loc, 1, GL_INT,						stride, (void*)(uptr)attr.offset);	break;
			case IV2:			glVertexAttribIPointer(loc, 2, GL_INT,						stride, (void*)(uptr)attr.offset);	break;
			case IV3:			glVertexAttribIPointer(loc, 3, GL_INT,						stride, (void*)(uptr)attr.offset);	break;
			case IV4:			glVertexAttribIPointer(loc, 4, GL_INT,						stride, (void*)(uptr)attr.offset);	break;

			case U8V4_AS_FV4:	glVertexAttribPointer(loc, 4, GL_UNSIGNED_BYTE,	GL_TRUE,	stride, (void*)(uptr)attr.offset);	break;
		}
	}

	std::vector<Vertex_Attribute> attributes;

	void bind (Shader const& shad) const { // bind a vertex layout which requires the used shader

		static int max_enabled_attributes_loc = 0;
		for (int loc=(int)attributes.size(); loc <= max_enabled_attributes_loc; ++loc)
			glDisableVertexAttribArray(loc);

		for (auto& attr : attributes) {

			auto loc = glGetAttribLocation(shad.get_prog_handle(), attr.name.c_str());
			if (loc < 0)
				continue;

			max_enabled_attributes_loc = max(max_enabled_attributes_loc, loc +1);

			setup_attrib(loc, attr, vertex_size);
		}

	}
};

////
struct Gpu_Mesh;

template <typename VERT, typename INDX=u16>
struct Cpu_Mesh {
	std::vector<VERT>	vertecies;
	std::vector<INDX>	indecies;

	Gpu_Mesh upload (); // for convenience

	void clear () {
		vertecies.clear();
		indecies .clear();
	}

	void add (Cpu_Mesh<VERT> const& r) {
		auto l_verts = vertecies.size();

		auto l_indxs = indecies.size();
		auto r_indxs = r.indecies.size();

		indecies.resize(l_indxs +r_indxs);
		for (int i=0; i<r_indxs; ++i) {
			auto new_index = r.indecies[i] +l_verts;
			if (new_index > std::numeric_limits<INDX>::max()) {
				errprint("Error: index overflow in Cpu_Mesh::add!\n");
				assert(false);
			}
			indecies[l_indxs +i] = (INDX)new_index;
		}

		vertecies.insert(vertecies.end(), r.vertecies.begin(),r.vertecies.end());
	}
};

enum primitive_e : GLenum {
	TRIANGLES = GL_TRIANGLES,
	POINTS = GL_POINTS,
	LINES = GL_LINES,
};

struct Gpu_Mesh {
	Vertex_Layout const*	layout = nullptr;
	
	enum index_type_e : GLenum {
		UNSIGNED_SHORT = GL_UNSIGNED_SHORT,
		UNSIGNED_INT = GL_UNSIGNED_INT,
	};
	index_type_e			index_type = (index_type_e)0;

	VBO						vertecies;
	EBO						indecies;

	GLuint					vertex_count = 0;
	GLuint					index_count = 0;

	bool is_indexed () const { return index_count != 0; }
	bool _indecies_inited () const { return indecies.get_handle() != 0; }

	static int get_index_size_bytes (index_type_e index_size) {
		assert(index_size == UNSIGNED_SHORT || index_size == UNSIGNED_INT);
		switch (index_size) {
			case UNSIGNED_SHORT:	return 2; // bytes
			case UNSIGNED_INT:		return 4; // bytes
			default:				assert(false); return -1;
		}
	}

	template <typename T> static constexpr index_type_e map_index_type ();
	template <> static constexpr index_type_e map_index_type<u16> () { return UNSIGNED_SHORT; }
	template <> static constexpr index_type_e map_index_type<u32> () { return UNSIGNED_INT; }

	template <typename VERT, typename INDX>
	static Gpu_Mesh generate () {
		Gpu_Mesh m;
		m.layout = &VERT::layout;
		m.index_type = map_index_type<INDX>();

		m.vertecies = VBO::generate();
		// leave m.indecies null and lazy init if needed

		m.vertex_count = 0;
		m.index_count = 0;
		return m;
	}
	static Gpu_Mesh upload (void const* vertecies, GLuint vertex_count,
							void const* indecies, GLuint index_count,
							Vertex_Layout const* layout, index_type_e index_size) {
		Gpu_Mesh m;
		m.layout = layout;
		m.index_type = index_size;

		m.vertex_count = vertex_count;
		m.vertecies = VBO::gen_and_upload(vertecies, vertex_count * layout->vertex_size);

		m.index_count = index_count;
		if (m.is_indexed())
			m.indecies = EBO::gen_and_upload(indecies, index_count * get_index_size_bytes(index_size));

		return m;
	}
	void reupload (void const* vertecies, GLuint vertex_count,
							void const* indecies, GLuint index_count,
							Vertex_Layout const* layout, index_type_e index_size) {
		assert(this->layout == layout);
		assert(this->index_type == index_size);

		this->vertex_count = vertex_count;
		this->vertecies.reupload(vertecies, vertex_count * layout->vertex_size);

		this->index_count = index_count;

		if (is_indexed() && !_indecies_inited()) // lazy init
			this->indecies = EBO::generate();
		
		if (_indecies_inited())
			this->indecies.reupload(indecies, index_count * get_index_size_bytes(index_size));
	}

	template <typename VERT, typename INDX>
	static Gpu_Mesh upload (Cpu_Mesh<VERT,INDX> const& mesh) {
		return upload(mesh.vertecies.data(), (GLuint)mesh.vertecies.size(), mesh.indecies.data(), (GLuint)mesh.indecies.size(), &VERT::layout, map_index_type<INDX>());
	}
	template <typename VERT, typename INDX>
	void reupload (Cpu_Mesh<VERT,INDX> const& mesh) {
		return reupload(mesh.vertecies.data(), (GLuint)mesh.vertecies.size(), mesh.indecies.data(), (GLuint)mesh.indecies.size(), &VERT::layout, map_index_type<INDX>());
	}

	void bind (Shader const& shad) const {
		assert(vertecies.get_handle() != 0);

		static GLuint prev_vbo_handle = 0;
		static GLuint prev_shader_handle = 0;

		if (vertecies.get_handle() != prev_vbo_handle || shad.get_prog_handle() != prev_shader_handle) {

			vertecies.bind();

			if (is_indexed())
				indecies.bind();
			
			layout->bind(shad);

		}
		prev_vbo_handle = vertecies.get_handle();
		prev_shader_handle = shad.get_prog_handle();
	}

	void draw (primitive_e prim, Shader const& shad) const { // draws entire buffer
		bind(shad);

		if (is_indexed())
			glDrawElements(prim, (GLsizei)index_count, index_type, (void*)0);
		else
			glDrawArrays(prim, 0, (GLsizei)vertex_count);
	}
	void draw (Shader const& shad) const { // draws entire buffer
		draw(TRIANGLES, shad);
	}

	void draw (Shader const& shad, GLint first, GLsizei count) const { // draws subportion of buffer
		bind(shad);

		if (is_indexed()) {
			assert(first >= 0 && first < (GLint)index_count);
			assert(count >= 0 && count <= (GLsizei)index_count);
			glDrawElements(GL_TRIANGLES, count, index_type, (void*)(uptr)(first * get_index_size_bytes(index_type)));
		} else {
			assert(first >= 0 && first < (GLint)vertex_count);
			assert(count >= 0 && count <= (GLsizei)vertex_count);
			glDrawArrays(GL_TRIANGLES, first, count);
		}
	}
};

template <typename VERT,typename INDX> Gpu_Mesh Cpu_Mesh<VERT,INDX>::upload () {
	return Gpu_Mesh::upload(*this);
}

////
// helper to generate meshes for certain geometric objects
//  gen_vert will be called for each vertex and will be passed useful values depending on geometric object

template <typename VERT, typename INDX=u16, typename GEN_VERT> Cpu_Mesh<VERT,INDX> gen_rect (GEN_VERT gen_vert, v2 radius=0.5f, v2 center=0) { // GEN_VERT:  VERT func (v2 pos, v2 uv)
	Cpu_Mesh<VERT,INDX> m;
	for (auto uv : { v2(0,0),v2(1,0),v2(1,1),v2(0,1) })
		m.vertecies.push_back( gen_vert(lerp(-radius,+radius, uv) +center, uv) );
	
	m.indecies = { 1,2,0, 0,2,3 };
	return m;
}

template <typename VERT, typename INDX=u16, typename GEN_VERT> Cpu_Mesh<VERT,INDX> gen_cube (GEN_VERT gen_vert, v3 radius=0.5f, v3 center=0) { // GEN_VERT:  VERT func (v3 pos, v3 normal, v2 uv_face, int face_indx)

	struct Cube_Prototype_Vert {
		v3 pos;
		v3 normal;
		v2 uv_face; // uv within face, can be used to generate "real" uv, depending on how you want to use your cube
	};

	#if 0
	auto _gen_cube_verts = [] () {

		m3 face_rot[] = {
			rotate3_X(deg(90)) *	rotate3_Y(deg(90))	,
			rotate3_X(deg(90)) *	rotate3_Y(deg(-90))	,
			rotate3_Y(deg(180)) *	rotate3_X(deg(-90))	,
			rotate3_X(deg(90))							,
			m3::ident()									,
			rotate3_Z(deg(180)) *	rotate3_X(deg(180))	,
		};

		Cpu_Mesh<Cube_Prototype_Vert> mesh;

		for (auto mat : face_rot) {
			auto mat_ = mat; // cannot capture variables in for (...) ??
			auto face = gen_rect<Cube_Prototype_Vert>([&] (v2 p, v2 uv) {
				return Cube_Prototype_Vert{ mat_ * v3(p,1), mat_ * v3(0,0,+1), uv };
			}, 1);

			mesh.add(face);
		}

		printf("constexpr Cube_Prototype_Vert cube_vertecies[6*4] = {");
		for (int i=0; i<(int)mesh.vertecies.size(); ++i) {
			if (i % 4 == 0) printf("\n"); // each face on seperate line
			
			auto print_vert = [&] (v3 p, v3 n, v2 uv) {
				auto round = [] (float f) { return abs(f) < 0.00001f ? 0 : f; };

				printf("\t{ v3(%+g,%+g,%+g), v3(%+g,%+g,%+g), v2(%+g,%+g) },",
					round(p.x),
					round(p.y),
					round(p.z),

					round(n.x),
					round(n.y),
					round(n.z),

					round(uv.x),
					round(uv.y));
			};
			
			auto v = mesh.vertecies[i];
			print_vert(v.pos, v.normal, v.uv_face);
		}
		printf("\n};\n");

		printf("constexpr u16 cube_indecies[6*6] = {");
		for (int i=0; i<(int)mesh.indecies.size(); ++i) {
			if (i % 6 == 0) printf("\n\t"); // each face on seperate line
			printf("%d, ", mesh.indecies[i]);
		}
		printf("\n};\n");

		return true;
	};
	static bool _run_once = _gen_cube_verts();
	#endif
	
	// Generated via the code above
	static constexpr Cube_Prototype_Vert cube_vertecies[6*4] = {
		{ v3(+1,-1,-1), v3(+1,+0,+0), v2(+0,+0) },      { v3(+1,+1,-1), v3(+1,+0,+0), v2(+1,+0) },      { v3(+1,+1,+1), v3(+1,+0,+0), v2(+1,+1) },      { v3(+1,-1,+1), v3(+1,+0,+0), v2(+0,+1) },
		{ v3(-1,+1,-1), v3(-1,+0,+0), v2(+0,+0) },      { v3(-1,-1,-1), v3(-1,+0,+0), v2(+1,+0) },      { v3(-1,-1,+1), v3(-1,+0,+0), v2(+1,+1) },      { v3(-1,+1,+1), v3(-1,+0,+0), v2(+0,+1) },
		{ v3(+1,+1,-1), v3(+0,+1,+0), v2(+0,+0) },      { v3(-1,+1,-1), v3(+0,+1,+0), v2(+1,+0) },      { v3(-1,+1,+1), v3(+0,+1,+0), v2(+1,+1) },      { v3(+1,+1,+1), v3(+0,+1,+0), v2(+0,+1) },
		{ v3(-1,-1,-1), v3(+0,-1,+0), v2(+0,+0) },      { v3(+1,-1,-1), v3(+0,-1,+0), v2(+1,+0) },      { v3(+1,-1,+1), v3(+0,-1,+0), v2(+1,+1) },      { v3(-1,-1,+1), v3(+0,-1,+0), v2(+0,+1) },
		{ v3(-1,-1,+1), v3(+0,+0,+1), v2(+0,+0) },      { v3(+1,-1,+1), v3(+0,+0,+1), v2(+1,+0) },      { v3(+1,+1,+1), v3(+0,+0,+1), v2(+1,+1) },      { v3(-1,+1,+1), v3(+0,+0,+1), v2(+0,+1) },
		{ v3(+1,-1,-1), v3(+0,+0,-1), v2(+0,+0) },      { v3(-1,-1,-1), v3(+0,+0,-1), v2(+1,+0) },      { v3(-1,+1,-1), v3(+0,+0,-1), v2(+1,+1) },      { v3(+1,+1,-1), v3(+0,+0,-1), v2(+0,+1) },
	};
	static constexpr u16 cube_indecies[6*6] = {
		1, 2, 0, 0, 2, 3,
		5, 6, 4, 4, 6, 7,
		9, 10, 8, 8, 10, 11,
		13, 14, 12, 12, 14, 15,
		17, 18, 16, 16, 18, 19,
		21, 22, 20, 20, 22, 23,
	};

	Cpu_Mesh<VERT,INDX> m;
	for (int face=0; face<6; ++face) {
		for (int i=0; i<4; ++i) {
			auto& v = cube_vertecies[face*4+ i];
			m.vertecies.push_back( gen_vert(v.pos * radius +center, v.normal, v.uv_face, face) );
		}
	}
	m.indecies.assign(&cube_indecies[0], &cube_indecies[6*6]);

	return m;
}

////
struct Default_Vertex_2d {
	v2		pos_model;
	v2		uv				= 0.5f;
	lrgba	col_lrgba		= lrgba(white, 1);

	static const Vertex_Layout layout;
};
const Vertex_Layout Default_Vertex_2d::layout = { (int)sizeof(Default_Vertex_2d), {
	{ "pos_model",			FV2,	(int)offsetof(Default_Vertex_2d, pos_model) },
	{ "uv",					FV2,	(int)offsetof(Default_Vertex_2d, uv) },
	{ "col_lrgba",			FV4,	(int)offsetof(Default_Vertex_2d, col_lrgba) }
}};

struct Default_Vertex_3d {
	v3		pos_model;
	v3		normal_model	= v3(0, 0,+1);
	v4		tangent_model	= v4(0,+1, 0,+1);
	v2		uv				= 0.5f;
	lrgba	col_lrgba		= lrgba(white, 1);

	static const Vertex_Layout layout;
};
const Vertex_Layout Default_Vertex_3d::layout = { (int)sizeof(Default_Vertex_3d), {
	{ "pos_model",			FV3,	(int)offsetof(Default_Vertex_3d, pos_model) },
	{ "normal_model",		FV3,	(int)offsetof(Default_Vertex_3d, normal_model) },
	{ "tangent_model",		FV4,	(int)offsetof(Default_Vertex_3d, tangent_model) },
	{ "uv",					FV2,	(int)offsetof(Default_Vertex_3d, uv) },
	{ "col_lrgba",			FV4,	(int)offsetof(Default_Vertex_3d, col_lrgba) }
}};

//
}
using namespace vertex_layout;
}
