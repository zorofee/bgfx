#pragma once

#include <array>
#include <string>
#include "bgfx/bgfx.h"
#include "vulkan-local/vulkan_core.h"
#include "nvpro_core/nvh/gltfscene.h"
#include "nvpro_core/nvh/nvprint.h"
#include "nvpro_core/nvvk/commands_vk.h"
#include "nvpro_core/nvvk/debug_util_vk.h"
#include "nvpro_core/nvvk/resource_allocator.h"
#include "shaders/host_device.h"
#include "queue.h"


class RayTracingScene
{
public:
	enum EBuffer
	{
		eCameraMat,
		eMaterial,
		eInstData,
		eLights,
	};

	enum EBuffers
	{
		eVertex,
		eIndex,
		eLast_elem
	};

	void setup(const VkDevice& device, const VkPhysicalDevice& physicalDevice, uint32_t familyIndex, VkQueue queue, bgfx::ResourceAllocator* allocator);
	// vbo组织方法，组织为GltfPrimMesh
	void initRayTracingScene(void* verticesData, void* indicesData);
	void createMaterialBuffer(VkCommandBuffer cmdBuf, const bgfx::GltfScene& gltf);
	void createVertexBuffer(VkCommandBuffer cmdBuf, const bgfx::GltfScene& gltf);
	void destroy();


	bgfx::GltfScene&					getScene() { return m_gltf; }
	const std::vector<bgfx::Buffer>&	getBuffers(EBuffers b) { return m_buffers[b]; }

private:

	// Setup
	bgfx::ResourceAllocator* m_pAlloc;  // Allocator for buffer, images, acceleration structures
	bgfx::DebugUtil          m_debug;   // Utility to name objects
	VkDevice                 m_device;
	//bgfx::Queue              m_queue;
	uint32_t                 m_queueFamilyIndex{ 0 };
	VkQueue					 m_queue;

	bgfx::GltfScene m_gltf;
	bgfx::GltfStats m_stats;

	std::array<bgfx::Buffer, 5>                            m_buffer;           // For single buffer
	std::array<std::vector<bgfx::Buffer>, 2>               m_buffers;          // For array of buffers (vertex/index)

	void importDrawableNodes(bgfx::VertexBufferHandle vbh, bgfx::IndexBufferHandle ibh);
};
