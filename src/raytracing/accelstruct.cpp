
#include "accelstruct.h"
#include "nvpro_core/nvvk/raytraceKHR_vk.h"
#include "shaders/host_device.h"
#include "tools.h"

#include <sstream>
#include <ios>

namespace bgfx
{

	void AccelStructure::setup(const VkDevice& device, const VkPhysicalDevice& physicalDevice, uint32_t familyIndex, bgfx::ResourceAllocator* allocator)
	{
		m_device = device;
		m_pAlloc = allocator;
		m_queueIndex = familyIndex;
		m_debug.setup(device);
		m_rtBuilder.setup(m_device, allocator, familyIndex);
	}


	void AccelStructure::destroy()
	{
		m_rtBuilder.destroy();
		BGFX_VKAPI(vkDestroyDescriptorPool)(m_device, m_rtDescPool, nullptr);
		BGFX_VKAPI(vkDestroyDescriptorSetLayout)(m_device, m_rtDescSetLayout, nullptr);
	}

	void AccelStructure::create(bgfx::GltfScene& gltfScene, const std::vector<bgfx::Buffer>& vertex, const std::vector<bgfx::Buffer>& index)
	{
		MilliTimer timer;
		LOGI("Create acceleration structure \n");
		destroy();  // reset

		std::vector<uint32_t> vertexCounts;
		std::vector<uint32_t> indexCounts;
		for (bgfx::GltfPrimMesh& primMesh : gltfScene.m_primMeshes)
		{
			vertexCounts.push_back(primMesh.vertexCount);
			indexCounts.push_back(primMesh.indexCount);
		}

		std::vector<int> nodePrimMeshes;
		std::vector<VkTransformMatrixKHR> nodeMeshTransform;
		for (bgfx::GltfNode& node : gltfScene.m_nodes)
		{
			nodePrimMeshes.push_back(node.primMesh);
			nodeMeshTransform.push_back(bgfx::toTransformMatrixKHR(node.worldMatrix));
		}
		//createBottomLevelAS(gltfScene, vertex, index);
		createBottomLevelAS(vertexCounts,indexCounts,vertex,index);
		//createTopLevelAS(gltfScene);
		createTopLevelAS(nodePrimMeshes, nodeMeshTransform, gltfScene);
		createRtDescriptorSet();
		timer.print();
	}


	//--------------------------------------------------------------------------------------------------
	// Converting a GLTF primitive in the Raytracing Geometry used for the BLAS
	//
	bgfx::RaytracingBuilderKHR::BlasInput AccelStructure::primitiveToGeometry(const bgfx::GltfPrimMesh& prim, VkBuffer vertex, VkBuffer index)
	{
		// Building part
		VkBufferDeviceAddressInfo info{ VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
		info.buffer = vertex;
		VkDeviceAddress vertexAddress = BGFX_VKAPI(vkGetBufferDeviceAddress)(m_device, &info);
		info.buffer = index;
		VkDeviceAddress indexAddress = BGFX_VKAPI(vkGetBufferDeviceAddress)(m_device, &info);

		VkAccelerationStructureGeometryTrianglesDataKHR triangles{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR };
		triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
		triangles.vertexData.deviceAddress = vertexAddress;
		triangles.vertexStride = sizeof(VertexAttributes);
		triangles.indexType = VK_INDEX_TYPE_UINT32;
		triangles.indexData.deviceAddress = indexAddress;
		triangles.maxVertex = prim.vertexCount;

		// Setting up the build info of the acceleration
		VkAccelerationStructureGeometryKHR asGeom{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
		asGeom.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
		asGeom.flags = VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR;  // For AnyHit
		asGeom.geometry.triangles = triangles;

		VkAccelerationStructureBuildRangeInfoKHR offset;
		offset.firstVertex = 0;
		offset.primitiveCount = prim.indexCount / 3;
		offset.primitiveOffset = 0;
		offset.transformOffset = 0;

		bgfx::RaytracingBuilderKHR::BlasInput input;
		input.asGeometry.emplace_back(asGeom);
		input.asBuildOffsetInfo.emplace_back(offset);
		return input;
	}
	//--------------------------------------------------------------------------------------------------
	bgfx::RaytracingBuilderKHR::BlasInput AccelStructure::primitiveToGeometry(uint32_t vertexCount, uint32_t indexCount, VkBuffer vertex, VkBuffer index)
	{
		// Building part
		VkBufferDeviceAddressInfo info{ VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
		info.buffer = vertex;
		VkDeviceAddress vertexAddress = BGFX_VKAPI(vkGetBufferDeviceAddress)(m_device, &info);
		info.buffer = index;
		VkDeviceAddress indexAddress = BGFX_VKAPI(vkGetBufferDeviceAddress)(m_device, &info);

		VkAccelerationStructureGeometryTrianglesDataKHR triangles{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR };
		triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
		triangles.vertexData.deviceAddress = vertexAddress;
		triangles.vertexStride = sizeof(VertexAttributes);
		triangles.indexType = VK_INDEX_TYPE_UINT32;
		triangles.indexData.deviceAddress = indexAddress;
		triangles.maxVertex = vertexCount;

		// Setting up the build info of the acceleration
		VkAccelerationStructureGeometryKHR asGeom{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
		asGeom.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
		asGeom.flags = VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR;  // For AnyHit
		asGeom.geometry.triangles = triangles;

		VkAccelerationStructureBuildRangeInfoKHR offset;
		offset.firstVertex = 0;
		offset.primitiveCount = indexCount / 3;
		offset.primitiveOffset = 0;
		offset.transformOffset = 0;

		bgfx::RaytracingBuilderKHR::BlasInput input;
		input.asGeometry.emplace_back(asGeom);
		input.asBuildOffsetInfo.emplace_back(offset);
		return input;
	}
	//--------------------------------------------------------------------------------------------------
	//
	// uint32_t primMeshesSize,
	void AccelStructure::createBottomLevelAS(bgfx::GltfScene& gltfScene,
		const std::vector<bgfx::Buffer>& vertex,
		const std::vector<bgfx::Buffer>& index)
	{
		// BLAS - Storing each primitive in a geometry
		uint32_t                                           prim_idx{ 0 };
		std::vector<bgfx::RaytracingBuilderKHR::BlasInput> allBlas;
		allBlas.reserve(gltfScene.m_primMeshes.size());
		// input: VBOs, 可以在gltfScene中组织为m_primMeshes
		for (bgfx::GltfPrimMesh& primMesh : gltfScene.m_primMeshes)
		{
			auto geo = primitiveToGeometry(primMesh.vertexCount,primMesh.indexCount, vertex[prim_idx].buffer, index[prim_idx].buffer);
			allBlas.push_back({ geo });
			prim_idx++;
		}
		LOGI(" BLAS(%d)\n", allBlas.size());
		m_rtBuilder.buildBlas(allBlas, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR
			| VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR);
	}
	//--------------------------------------------------------------------------------------------------
	void AccelStructure::createBottomLevelAS(const std::vector<uint32_t>& vertexCounts,
		const std::vector<uint32_t>& indexCounts,
		const std::vector<bgfx::Buffer>& vertex,
		const std::vector<bgfx::Buffer>& index)
	{
		size_t primMeshCount = vertexCounts.size();
		std::vector<bgfx::RaytracingBuilderKHR::BlasInput> allBlas;
		allBlas.reserve(primMeshCount);
		for (size_t i = 0; i < primMeshCount; i++)
		{
			auto geo = primitiveToGeometry(vertexCounts[i], indexCounts[i], vertex[i].buffer, index[i].buffer);
			allBlas.push_back({ geo });
		}
		LOGI(" BLAS(%d)\n", allBlas.size());
		m_rtBuilder.buildBlas(allBlas, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR
			| VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR);
	}
	//--------------------------------------------------------------------------------------------------
	//
	//
	void AccelStructure::createTopLevelAS(bgfx::GltfScene& gltfScene)
	{
		std::vector<VkAccelerationStructureInstanceKHR> tlas;
		tlas.reserve(gltfScene.m_nodes.size());
		
		for (auto& node : gltfScene.m_nodes)
		{
			// Flags
			VkGeometryInstanceFlagsKHR flags{};
			bgfx::GltfPrimMesh& primMesh	= gltfScene.m_primMeshes[node.primMesh];
			bgfx::GltfMaterial& mat			= gltfScene.m_materials[primMesh.materialIndex];
			// Always opaque, no need to use anyhit (faster)
			if (mat.alphaMode == 0 || (mat.baseColorFactor.w == 1.0f && mat.baseColorTexture == -1))
				flags |= VK_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT_KHR;
			// Need to skip the cull flag in traceray_rtx for double sided materials
			if (mat.doubleSided == 1)
				flags |= VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;

			VkAccelerationStructureInstanceKHR rayInst{};
			rayInst.transform								= bgfx::toTransformMatrixKHR(node.worldMatrix);
			rayInst.instanceCustomIndex						= node.primMesh;  // gl_InstanceCustomIndexEXT: to find which primitive
			rayInst.accelerationStructureReference			= m_rtBuilder.getBlasDeviceAddress(node.primMesh);
			rayInst.flags									= flags;
			rayInst.instanceShaderBindingTableRecordOffset	= 0;  // We will use the same hit group for all objects
			rayInst.mask									= 0xFF;
			tlas.emplace_back(rayInst);
		}
		LOGI(" TLAS(%d)", tlas.size());
		m_rtBuilder.buildTlas(tlas, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
	}
	//--------------------------------------------------------------------------------------------------
	void AccelStructure::createTopLevelAS(const std::vector<int>& nodePrimMeshes, const std::vector<VkTransformMatrixKHR>& worldMatrix,  bgfx::GltfScene& gltfScene)
	{
		std::vector<VkAccelerationStructureInstanceKHR> tlas;
		tlas.reserve(nodePrimMeshes.size());
		for (size_t i = 0; i < nodePrimMeshes.size(); i++)
		{
			VkGeometryInstanceFlagsKHR flags{};
			bgfx::GltfPrimMesh& primMesh = gltfScene.m_primMeshes[nodePrimMeshes[i]];
			bgfx::GltfMaterial& mat = gltfScene.m_materials[primMesh.materialIndex];
			// Always opaque, no need to use anyhit (faster)
			if (mat.alphaMode == 0 || (mat.baseColorFactor.w == 1.0f && mat.baseColorTexture == -1))
				flags |= VK_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT_KHR;
			// Need to skip the cull flag in traceray_rtx for double sided materials
			if (mat.doubleSided == 1)
				flags |= VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;


			VkAccelerationStructureInstanceKHR rayInst{};
			rayInst.transform = worldMatrix[i];
			rayInst.instanceCustomIndex = nodePrimMeshes[i];  // gl_InstanceCustomIndexEXT: to find which primitive
			rayInst.accelerationStructureReference = m_rtBuilder.getBlasDeviceAddress(nodePrimMeshes[i]);
			rayInst.flags = flags;
			rayInst.instanceShaderBindingTableRecordOffset = 0;  // We will use the same hit group for all objects
			rayInst.mask = 0xFF;
			tlas.emplace_back(rayInst);
		}
		LOGI(" TLAS(%d)", tlas.size());
		m_rtBuilder.buildTlas(tlas, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
	}

	//--------------------------------------------------------------------------------------------------
	// Descriptor set holding the TLAS
	//
	void AccelStructure::createRtDescriptorSet()
	{
		VkShaderStageFlags flags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR
			| VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		bgfx::DescriptorSetBindings bind;
		bind.addBinding({ AccelBindings::eTlas, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1, flags });  // TLAS

		m_rtDescPool = bind.createPool(m_device);
		CREATE_NAMED_VK(m_rtDescSetLayout, bind.createLayout(m_device));
		CREATE_NAMED_VK(m_rtDescSet, bgfx::allocateDescriptorSet(m_device, m_rtDescPool, m_rtDescSetLayout));

		VkAccelerationStructureKHR tlas = m_rtBuilder.getAccelerationStructure();

		VkWriteDescriptorSetAccelerationStructureKHR descASInfo{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR };
		descASInfo.accelerationStructureCount = 1;
		descASInfo.pAccelerationStructures = &tlas;

		std::vector<VkWriteDescriptorSet> writes;
		writes.emplace_back(bind.makeWrite(m_rtDescSet, AccelBindings::eTlas, &descASInfo));
		BGFX_VKAPI(vkUpdateDescriptorSets)(m_device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
	}

}
