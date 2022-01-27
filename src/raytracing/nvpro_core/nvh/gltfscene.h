#pragma once
#pragma once
#include "../nvmath/nvmath.h"
//#include "tiny_gltf.h"
#include <algorithm>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>


namespace bgfx
{
	struct GltfPrimMesh
	{
		uint32_t firstIndex{ 0 };
		uint32_t indexCount{ 0 };
		uint32_t vertexOffset{ 0 };
		uint32_t vertexCount{ 0 };
		int      materialIndex{ 0 };

		nvmath::vec3f posMin{ 0, 0, 0 };
		nvmath::vec3f posMax{ 0, 0, 0 };
		std::string   name;
	};

	//--------------------------------------------------------------------------------------------------
// Class to convert gltfScene in simple draw-able format
//
	struct GltfScene
	{
		//void importMaterials(const tinygltf::Model& tmodel);
		//void importDrawableNodes(const tinygltf::Model& tmodel, GltfAttributes attributes);
		void computeSceneDimensions();
		void destroy();

		//static GltfStats getStatistics(const tinygltf::Model& tinyModel);

		// Scene data
		//std::vector<GltfMaterial> m_materials;   // Material for shading
		//std::vector<GltfNode>     m_nodes;       // Drawable nodes, flat hierarchy
		std::vector<GltfPrimMesh> m_primMeshes;  // Primitive promoted to meshes
		//std::vector<GltfCamera>   m_cameras;
		//std::vector<GltfLight>    m_lights;

		// Attributes, all same length if valid
		std::vector<nvmath::vec3f> m_positions;
		std::vector<uint32_t>      m_indices;
		std::vector<nvmath::vec3f> m_normals;
		std::vector<nvmath::vec4f> m_tangents;
		std::vector<nvmath::vec2f> m_texcoords0;
		std::vector<nvmath::vec2f> m_texcoords1;
		std::vector<nvmath::vec4f> m_colors0;

		// #TODO - Adding support for Skinning
		//using vec4us = vector4<unsigned short>;
		//std::vector<vec4us>        m_joints0;
		//std::vector<nvmath::vec4f> m_weights0;

		// Size of the scene
		struct Dimensions
		{
			nvmath::vec3f min = nvmath::vec3f(std::numeric_limits<float>::max());
			nvmath::vec3f max = nvmath::vec3f(std::numeric_limits<float>::min());
			nvmath::vec3f size{ 0.f };
			nvmath::vec3f center{ 0.f };
			float         radius{ 0 };
		} m_dimensions;


	private:
		//void processNode(const tinygltf::Model& tmodel, int& nodeIdx, const nvmath::mat4f& parentMatrix);
		//void processMesh(const tinygltf::Model& tmodel, const tinygltf::Primitive& tmesh, GltfAttributes attributes, const std::string& name);

		// Temporary data
		std::unordered_map<int, std::vector<uint32_t>> m_meshToPrimMeshes;
		std::vector<uint32_t>                          primitiveIndices32u;
		std::vector<uint16_t>                          primitiveIndices16u;
		std::vector<uint8_t>                           primitiveIndices8u;

		std::unordered_map<std::string, GltfPrimMesh> m_cachePrimMesh;

		void computeCamera();
		//void checkRequiredExtensions(const tinygltf::Model& tmodel);
	};
}
