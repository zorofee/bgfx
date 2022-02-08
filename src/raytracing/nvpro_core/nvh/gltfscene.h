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
	struct rawVertex
	{
		float m_x;
		float m_y;
		float m_z;
		float m_nx;
		float m_ny;
		float m_nz;
		float m_tx;
		float m_ty;
		float m_tz;
		int16_t m_u;
		int16_t m_v;
	};

	struct GltfMaterial
	{
		int shadingModel{ 0 };  // 0: metallic-roughness, 1: specular-glossiness

		// pbrMetallicRoughness
		nvmath::vec4f baseColorFactor{ 1.f, 1.f, 1.f, 1.f };
		int           baseColorTexture{ -1 };
		float         metallicFactor{ 1.f };
		float         roughnessFactor{ 1.f };
		int           metallicRoughnessTexture{ -1 };

		int           emissiveTexture{ -1 };
		nvmath::vec3f emissiveFactor{ 0, 0, 0 };
		int           alphaMode{ 0 };
		float         alphaCutoff{ 0.5f };
		int           doubleSided{ 0 };

		int   normalTexture{ -1 };
		float normalTextureScale{ 1.f };
		int   occlusionTexture{ -1 };
		float occlusionTextureStrength{ 1 };

		// Extensions
		//KHR_materials_pbrSpecularGlossiness specularGlossiness;
		//KHR_texture_transform               textureTransform;
		//KHR_materials_clearcoat             clearcoat;
		//KHR_materials_sheen                 sheen;
		//KHR_materials_transmission          transmission;
		//KHR_materials_unlit                 unlit;
		//KHR_materials_anisotropy            anisotropy;
		//KHR_materials_ior                   ior;
		//KHR_materials_volume                volume;
	};

	struct GltfNode
	{
		nvmath::mat4f worldMatrix{ 1 };
		int           primMesh{ 0 };
	};

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

	struct GltfStats
	{
		uint32_t nbCameras{ 0 };
		uint32_t nbImages{ 0 };
		uint32_t nbTextures{ 0 };
		uint32_t nbMaterials{ 0 };
		uint32_t nbSamplers{ 0 };
		uint32_t nbNodes{ 0 };
		uint32_t nbMeshes{ 0 };
		uint32_t nbLights{ 0 };
		uint32_t imageMem{ 0 };
		uint32_t nbUniqueTriangles{ 0 };
		uint32_t nbTriangles{ 0 };
	};

	struct GltfCamera
	{
		nvmath::mat4f worldMatrix{ 1 };
		nvmath::vec3f eye{ 0, 0, 0 };
		nvmath::vec3f center{ 0, 0, 0 };
		nvmath::vec3f up{ 0, 1, 0 };

		//tinygltf::Camera cam;
	};

	struct GltfLight
	{
		nvmath::mat4f   worldMatrix{ 1 };
		//tinygltf::Light light;
	};

	//--------------------------------------------------------------------------------------------------
	// Class to convert gltfScene in simple draw-able format
	//
	struct GltfScene
	{
		void importMaterials();
		//void importDrawableNodes(const tinygltf::Model& tmodel, GltfAttributes attributes);
		void processRawVerticesData(void* verticesData, void* indicesData);
		void computeSceneDimensions();
		void destroy();

		static GltfStats getStatistics();

		// Scene data
		std::vector<GltfMaterial> m_materials;   // Material for shading
		std::vector<GltfNode>     m_nodes;       // Drawable nodes, flat hierarchy
		std::vector<GltfPrimMesh> m_primMeshes;  // Primitive promoted to meshes
		std::vector<GltfCamera>   m_cameras;
		std::vector<GltfLight>    m_lights;

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
