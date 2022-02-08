#include "gltfscene.h"
#include "../nvh/nvprint.h"
#include <iostream>
#include <numeric>
#include <limits>
#include <set>
#include <sstream>

namespace bgfx
{
	//--------------------------------------------------------------------------------------------------
	// 临时使用手写的cube材质
	// 
	void GltfScene::importMaterials()
	{
		m_materials.reserve(1);
		GltfMaterial gmat;

		gmat.alphaCutoff = 0.5;
		gmat.alphaMode = 0;
		gmat.doubleSided = 1;
		gmat.emissiveFactor = nvmath::vec3f(1.0, 1.0, 1.0);
		gmat.emissiveTexture = -1;
		gmat.normalTexture = -1;
		gmat.normalTextureScale = 1;
		gmat.occlusionTexture = -1;
		gmat.occlusionTextureStrength = 1;

		// PbrMetallicRoughness
		gmat.baseColorFactor =	nvmath::vec4f(1.0, 1.0, 1.0, 1.0);
		gmat.baseColorTexture = -1;
		gmat.metallicFactor = 0;
		gmat.metallicRoughnessTexture = -1;
		gmat.roughnessFactor = 0.5;

		// KHR material extentions...

		m_materials.emplace_back(gmat);
	}
	//--------------------------------------------------------------------------------------------------
	// 临时使用手写的cube顶点数据
	// 
	void GltfScene::processRawVerticesData(void* verticesData, void* indicesData)
	{
		// temporal
		uint16_t verticesCount = 24;
		uint16_t indicesCount = 36;

		bgfx::GltfPrimMesh resultMesh;
		rawVertex* rvData = (rawVertex*)verticesData;
		uint16_t* indices = (uint16_t*)indicesData;

		resultMesh.materialIndex = 0;
		resultMesh.vertexOffset = static_cast<uint32_t>(m_positions.size());
		resultMesh.firstIndex = static_cast<uint32_t>(m_indices.size());
		//--------------------------------------------------------------------------------------------------
		// process indices
		resultMesh.indexCount = indicesCount;
		primitiveIndices16u.resize(indicesCount);
		memcpy(primitiveIndices16u.data(), indicesData,
			indicesCount * sizeof(uint16_t));
		m_indices.insert(m_indices.end(), primitiveIndices16u.begin(), primitiveIndices16u.end());

		//--------------------------------------------------------------------------------------------------
		// process vertices
		resultMesh.vertexCount = verticesCount;
		// position
		std::vector<nvmath::vec3f> geoposition(resultMesh.vertexCount);
		std::vector<nvmath::vec3f> geotangent(resultMesh.vertexCount);
		std::vector<nvmath::vec2f> geotexcoord(resultMesh.vertexCount);
		for (size_t i = 0; i < resultMesh.vertexCount; i++)
		{
			geoposition[i]	= nvmath::vec3f(rvData[i].m_x, rvData[i].m_y, rvData[i].m_z);
			geotangent[i]	= nvmath::vec3f(rvData[i].m_tx, rvData[i].m_ty, rvData[i].m_tz);
			geotexcoord[i]	= nvmath::vec2f(rvData[i].m_u, rvData[i].m_v);
		}
		m_positions.insert(m_positions.end(), geoposition.begin(), geoposition.end());
		m_tangents.insert(m_tangents.end(), geotangent.begin(), geotangent.end());
		m_texcoords0.insert(m_texcoords0.end(), geotexcoord.begin(), geotexcoord.end());

		// normal
		std::vector<nvmath::vec3f> geonormal(resultMesh.vertexCount);
		for (size_t i = 0; i < resultMesh.indexCount; i += 3)
		{
			uint32_t    ind0 = m_indices[resultMesh.firstIndex + i + 0];
			uint32_t    ind1 = m_indices[resultMesh.firstIndex + i + 1];
			uint32_t    ind2 = m_indices[resultMesh.firstIndex + i + 2];
			const auto& pos0 = m_positions[ind0 + resultMesh.vertexOffset];
			const auto& pos1 = m_positions[ind1 + resultMesh.vertexOffset];
			const auto& pos2 = m_positions[ind2 + resultMesh.vertexOffset];
			const auto  v1 = nvmath::normalize(pos1 - pos0);  // Many normalize, but when objects are really small the
			const auto  v2 = nvmath::normalize(pos2 - pos0);  // cross will go below nv_eps and the normal will be (0,0,0)
			const auto  n = nvmath::cross(v2, v1);
			geonormal[ind0] += n;
			geonormal[ind1] += n;
			geonormal[ind2] += n;
		}
		for (auto& n : geonormal)
			n = nvmath::normalize(n);
		m_normals.insert(m_normals.end(), geonormal.begin(), geonormal.end());

		// COLOR_0
		m_colors0.insert(m_colors0.end(), resultMesh.vertexCount, nvmath::vec4f(1, 1, 1, 1));

		m_primMeshes.emplace_back(resultMesh);

		// 处理到一个 node 中
		bgfx::GltfNode node;
		node.worldMatrix = nvmath::mat4f(1);
		node.primMesh = 0;
		m_nodes.emplace_back(node);
	}

	//--------------------------------------------------------------------------------------------------
	// Retrieving information about the scene
	// 临时使用手写的 cube Stats 
	//
	GltfStats GltfScene::getStatistics()
	{
		GltfStats stats;

		stats.nbCameras = static_cast<uint32_t>(0);
		stats.nbImages = static_cast<uint32_t>(0);
		stats.nbTextures = static_cast<uint32_t>(0);
		stats.nbMaterials = static_cast<uint32_t>(1);
		stats.nbSamplers = static_cast<uint32_t>(0);
		stats.nbNodes = static_cast<uint32_t>(1);
		stats.nbMeshes = static_cast<uint32_t>(1);
		stats.nbLights = static_cast<uint32_t>(0);

		//...

		return stats;
	}

}

