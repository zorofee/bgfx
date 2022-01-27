
#include "accelstruct.h"
#include "nvpro_core/nvvk/raytraceKHR_vk.h"
#include "shaders/host_device.h"
//#include "tools.hpp"

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
		LOGI("Create acceleration structure \n");
		destroy();  // reset

		uint32_t primMeshesSize;
		createBottomLevelAS(gltfScene, vertex, index);
		createTopLevelAS();
		createRtDescriptorSet();
	}


	//--------------------------------------------------------------------------------------------------
	//
	// uint32_t vertexCount,  uint32_t indexCount, 
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
			auto geo = primitiveToGeometry(primMesh, vertex[prim_idx].buffer, index[prim_idx].buffer);
			allBlas.push_back({ geo });
			prim_idx++;
		}
		m_rtBuilder.buildBlas(allBlas, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR
			| VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR);
	}

	//--------------------------------------------------------------------------------------------------
	//
	//
	void AccelStructure::createTopLevelAS()
	{

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
