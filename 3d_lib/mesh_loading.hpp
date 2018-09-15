#pragma once

#include <unordered_map>

#include "vertex_buffers.hpp"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

namespace engine {
//

struct Mesh {
	std::vector<Default_Vertex_3d>	vbo_data;
	//std::vector<u16>				ebo_data;
};

bool load_mesh (std::string const& filepath, Mesh* mesh) {
	Assimp::Importer importer;

	auto* scene = importer.ReadFile(filepath,
		aiProcess_CalcTangentSpace       | 
		aiProcess_Triangulate            |
		aiProcess_JoinIdenticalVertices  |
		aiProcess_SortByPType );
	if (!scene)
		return false;

	assert(scene->mNumMeshes == 1);
	auto& m = *scene->mMeshes[0];
	
	assert(m.mPrimitiveTypes == aiPrimitiveType_TRIANGLE);
	assert(m.mNumFaces > 0);
	assert(m.mNumVertices > 0);
	
	for (uint i=0; i<m.mNumFaces; ++i) {
		auto& f = m.mFaces[i];
	
		assert(f.mNumIndices == 3);
	
		for (int i=0; i<3; ++i) {
			
			auto indx = f.mIndices[i];
			assert(indx >= 0 && indx < m.mNumVertices);
			
			Default_Vertex_3d v;

			assert(m.HasPositions());
			{
				auto& p = m.mVertices[indx];
				v.pos_model = v3(p.x,p.y,p.z);
			}

			if (m.HasNormals()) {
				auto& n = m.mNormals[indx];
				v.normal_model = v3(n.x,n.y,n.z);
			}

			if (m.HasTangentsAndBitangents()) {
				auto& t_ = m.mTangents[indx];
				auto& b_ = m.mBitangents[indx];

				v3 tangent = v3(t_.x,t_.y,t_.z);
				v3 bitangent = v3(b_.x,b_.y,b_.z);

				v3 expected_bitangent = cross(v.normal_model, tangent);
				v3 actual_bitangent = bitangent;
				
				flt bitangent_sign = -dot(expected_bitangent, actual_bitangent); // TODO: why is this negative ?
				assert(bitangent_sign != 0);

				bitangent_sign = normalize(bitangent_sign);

				v.tangent_model = v4(tangent, bitangent_sign);
			}

			if (m.HasTextureCoords(0)) {
				auto& uv = m.mTextureCoords[0][indx];
				v.uv = v2(uv.x,uv.y);
			}

			if (m.HasVertexColors(0)) { // not tested
				auto& c = *m.mColors[0];
				v.col_lrgba = v4( to_linear(v3(c.r,c.g,c.b)), c.a );
			}
			
			mesh->vbo_data.push_back(v);
		}
	}

	return true;
}

struct Mesh_Manager {
	
	std::unordered_map<std::string, Mesh> meshes;

	Mesh* get_mesh (std::string const& name) {
		auto shad = meshes.find(name);
		if (shad == meshes.end()) {
			Mesh m;
			if (!load_mesh(name, &m))
				return nullptr;
			shad = meshes.emplace(name, std::move(m)).first;
		}
		return &shad->second;
	}
};

Mesh_Manager mesh_manager;

//
}
