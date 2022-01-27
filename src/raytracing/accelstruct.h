#pragma once
#include "nvpro_core/nvh/gltfscene.h"
#include "nvpro_core/nvvk/resource_allocator.h"
#include "nvpro_core/nvvk/descriptorsets_vk.h"
#include "nvpro_core/nvvk/raytraceKHR_vk.h"
#include "functions_vk.h"

namespace bgfx
{
	class AccelStructure
	{

	public:
		void setup(const VkDevice& device, const VkPhysicalDevice& physicalDevice, uint32_t familyIndex, bgfx::ResourceAllocator* allocator);
		void destroy();
		void create(bgfx::GltfScene& gltfScene, const std::vector<bgfx::Buffer>& vertex, const std::vector<bgfx::Buffer>& index);

		VkAccelerationStructureKHR getTlas() { return m_rtBuilder.getAccelerationStructure(); }
		VkDescriptorSetLayout      getDescLayout() { return m_rtDescSetLayout; }
		VkDescriptorSet            getDescSet() { return m_rtDescSet; }

	private:
		bgfx::RaytracingBuilderKHR::BlasInput primitiveToGeometry(const bgfx::GltfPrimMesh& prim, VkBuffer vertex, VkBuffer index);
		void                                  createBottomLevelAS(bgfx::GltfScene& gltfScene, const std::vector<bgfx::Buffer>& vertex, const std::vector<bgfx::Buffer>& index);
		void                                  createTopLevelAS();
		void                                  createRtDescriptorSet();

		// Setup
		bgfx::ResourceAllocator* m_pAlloc{ nullptr };  // Allocator for buffer, images, acceleration structures
		bgfx::DebugUtil          m_debug;            // Utility to name objects
		VkDevice                 m_device{ nullptr };
		uint32_t                 m_queueIndex{ 0 };

		bgfx::RaytracingBuilderKHR m_rtBuilder;

		VkDescriptorPool      m_rtDescPool{ VK_NULL_HANDLE };
		VkDescriptorSetLayout m_rtDescSetLayout{ VK_NULL_HANDLE };
		VkDescriptorSet       m_rtDescSet{ VK_NULL_HANDLE };
	};
}
