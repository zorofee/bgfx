#include "raytracing_scene.h"
#include "shaders/compress.glsl"

void RayTracingScene::setup(const VkDevice& device, const VkPhysicalDevice& physicalDevice, uint32_t familyIndex, VkQueue queue, bgfx::ResourceAllocator* allocator)
{
	m_device = device;
	m_pAlloc = allocator;
	m_queueFamilyIndex = familyIndex;
	m_queue = queue;
	m_debug.setup(device);
}

//--------------------------------------------------------------------------------------------------
// Linearize the scene graph to world space nodes.
//
void RayTracingScene::importDrawableNodes(bgfx::VertexBufferHandle vbh, bgfx::IndexBufferHandle ibh)
{
	// Find the number of vertex(attributes) and index
	//uint32_t nbVert{0};
	uint32_t nbIndex{ 0 };
	uint32_t meshCnt{ 0 };  // use for mesh to new meshes
	uint32_t primCnt{ 0 };  //  "   "  "  "


}

//--------------------------------------------------------------------------------------------------
// Process raw data of vertices and indices, create vertexBuffer
// allocate buffers and create descriptor set for all resources
//
void RayTracingScene::initRayTracingScene(void* verticesData, void* indicesData)
{
	destroy();
	bgfx::GltfScene gltf;

	m_stats = gltf.getStatistics();

	// Extracting scene information to our format and adding, if missing, attributes such as tangent
	{
		LOGI("Convert to internal GLTF");
		gltf.importMaterials();
		gltf.processRawVerticesData(verticesData, indicesData);
	}

	// We are using a different index (1), to allow loading in a different queue/thread than the display (0) is using
	// Note: the GTC family queue is used because the nvvk::cmdGenerateMipmaps uses vkCmdBlitImage and this
	// command requires graphic queue and not only transfer.
	LOGI("Create Buffers\n");
	bgfx::CommandPool cmdBufGet(m_device, m_queueFamilyIndex, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, m_queue);
	VkCommandBuffer   cmdBuf = cmdBufGet.createCommandBuffer();

	createMaterialBuffer(cmdBuf, gltf);
	createVertexBuffer(cmdBuf, gltf);

	//// Descriptor set for all elements
	//createDescriptorSet(gltf);

	// Keeping minimal resources
	m_gltf.m_nodes		= gltf.m_nodes;
	m_gltf.m_primMeshes = gltf.m_primMeshes;
	m_gltf.m_materials	= gltf.m_materials;
	//m_gltf.m_dimensions = gltf.m_dimensions;
}


//--------------------------------------------------------------------------------------------------
// Create a buffer of all materials
// Most parameters are supported, and GltfShadeMaterial is GLSL packed compliant
// #TODO: compress the material, is it too large.
void RayTracingScene::createMaterialBuffer(VkCommandBuffer cmdBuf, const bgfx::GltfScene& gltf)
{
	LOGI(" - Create %d Material Buffer", gltf.m_materials.size());

	std::vector<GltfShadeMaterial> shadeMaterials;
	for (auto& m : gltf.m_materials)
	{
		GltfShadeMaterial smat{};
		smat.pbrBaseColorFactor				= m.baseColorFactor;
		smat.pbrBaseColorTexture			= m.baseColorTexture;
		smat.pbrMetallicFactor				= m.metallicFactor;
		smat.pbrRoughnessFactor				= m.roughnessFactor;
		smat.pbrMetallicRoughnessTexture	= m.metallicRoughnessTexture;
		smat.shadingModel					= m.shadingModel;
		smat.emissiveTexture				= m.emissiveTexture;
		smat.emissiveFactor					= m.emissiveFactor;
		smat.alphaMode						= m.alphaMode;
		smat.alphaCutoff					= m.alphaCutoff;
		smat.doubleSided					= m.doubleSided;
		smat.normalTexture					= m.normalTexture;
		smat.normalTextureScale				= m.normalTextureScale;
		// KHR_materials_pbrSpecularGlossiness
		//smat.khrDiffuseFactor = m.specularGlossiness.diffuseFactor;
		//smat.khrSpecularFactor = m.specularGlossiness.specularFactor;
		//smat.khrDiffuseTexture = m.specularGlossiness.diffuseTexture;
		//smat.khrGlossinessFactor = m.specularGlossiness.glossinessFactor;
		//smat.khrSpecularGlossinessTexture = m.specularGlossiness.specularGlossinessTexture;

		// KHR_texture_transform
		//smat.uvTransform = m.textureTransform.uvTransform;
		// KHR_materials_unlit
		//smat.unlit = m.unlit.active;
		// KHR_materials_transmission
		//smat.transmissionFactor = m.transmission.factor;
		//smat.transmissionTexture = m.transmission.texture;
		// KHR_materials_anisotropy
		//smat.anisotropy = m.anisotropy.factor;
		//smat.anisotropyDirection = m.anisotropy.direction;
		// KHR_materials_ior
		//smat.ior = m.ior.ior;
		// KHR_materials_volume
		//smat.attenuationColor = m.volume.attenuationColor;
		//smat.thicknessFactor = m.volume.thicknessFactor;
		//smat.thicknessTexture = m.volume.thicknessTexture;
		//smat.attenuationDistance = m.volume.attenuationDistance;
		// KHR_materials_clearcoat
		//smat.clearcoatFactor = m.clearcoat.factor;
		//smat.clearcoatRoughness = m.clearcoat.roughnessFactor;
		//smat.clearcoatTexture = m.clearcoat.texture;
		//smat.clearcoatRoughnessTexture = m.clearcoat.roughnessTexture;
		// KHR_materials_sheen
		//smat.sheen = packUnorm4x8(vec4(m.sheen.colorFactor, m.sheen.roughnessFactor));

		shadeMaterials.emplace_back(smat);
	}
	m_buffer[eMaterial] = m_pAlloc->createBuffer(cmdBuf, shadeMaterials, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
	NAME_VK(m_buffer[eMaterial].buffer);
}
//--------------------------------------------------------------------------------------------------
// Creating a buffer per primitive mesh (BLAS) containing all Vertex (pos, nrm, .. )
// and a buffer of index.
//
// We are compressing the data, because it makes a huge difference in the raytracer when accessing the
// data.
//
// normal and tangent are compressed using "A Survey of Efficient Representations for Independent Unit Vectors"
// http://jcgt.org/published/0003/02/01/paper.pdf
// The handiness of the tangent is stored in the less significant bit of the V component of the tcoord.
// Color is encoded on 32bit
//
void RayTracingScene::createVertexBuffer(VkCommandBuffer cmdBuf, const bgfx::GltfScene& gltf)
{
	LOGI(" - Create %d Vertex Buffers", gltf.m_primMeshes.size());

	std::vector<VertexAttributes> vertex{};
	std::vector<uint32_t>         indices;

	uint32_t prim_idx{ 0 };
	for (const bgfx::GltfPrimMesh& primMesh : gltf.m_primMeshes)
	{
		// todo
		bgfx::Buffer v_buffer;
		vertex.resize(primMesh.vertexCount);
		for (size_t v_ctx = 0; v_ctx < primMesh.vertexCount; v_ctx++)
		{
			size_t           idx = primMesh.vertexOffset + v_ctx;
			VertexAttributes v{};
			v.position	= gltf.m_positions[idx];
			v.normal	= compress_unit_vec(gltf.m_normals[idx]);
			v.tangent	= compress_unit_vec(gltf.m_tangents[idx]);
			v.texcoord	= gltf.m_texcoords0[idx];
			v.color		= packUnorm4x8(gltf.m_colors0[idx]);
			uint32_t value = floatBitsToUint(v.texcoord.y);
			if (gltf.m_tangents[idx].w > 0)
				value |= 1;  // set bit, H == +1
			else
				value &= ~1;  // clear bit, H == -1
			v.texcoord.y = uintBitsToFloat(value);
			vertex[v_ctx] = std::move(v);
		}
		v_buffer = m_pAlloc->createBuffer(cmdBuf, vertex,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
			| VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR);
		NAME_IDX_VK(v_buffer.buffer, prim_idx);

		// Buffer of indices
		indices.resize(primMesh.indexCount);
		for (size_t idx = 0; idx < primMesh.indexCount; idx++)
		{
			indices[idx] = gltf.m_indices[idx + primMesh.firstIndex];
		}

		bgfx::Buffer i_buffer = m_pAlloc->createBuffer(cmdBuf, indices,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
			| VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR);

		m_buffers[eVertex].push_back(v_buffer);
		NAME_IDX_VK(v_buffer.buffer, prim_idx);

		m_buffers[eIndex].push_back(i_buffer);
		NAME_IDX_VK(i_buffer.buffer, prim_idx);
		prim_idx++;
	}
}
//--------------------------------------------------------------------------------------------------
// Destroying all allocated resources
//
void RayTracingScene::destroy()
{
	for (auto& buffer : m_buffer)
	{
		m_pAlloc->destroy(buffer);
		buffer = {};
	}

	// This is to avoid deleting twice a buffer, the vector
	// of vertex buffer can be sharing buffers
	std::unordered_map<VkBuffer, bgfx::Buffer> map_bv;
	for (auto& buffers : m_buffers[eVertex])
		map_bv[buffers.buffer] = buffers;
	for (auto& bv : map_bv)
		m_pAlloc->destroy(bv.second);
	m_buffers[eVertex].clear();

	for (auto& buffers : m_buffers[eIndex])
	{
		m_pAlloc->destroy(buffers);
	}
	m_buffers[eIndex].clear();

	//...

	m_gltf = {};
}
