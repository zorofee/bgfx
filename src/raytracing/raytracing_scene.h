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

namespace bgfx
{

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
		bool load(const std::string& filename);
		bool loadGltfScene(const std::string& filename, tinygltf::Model& tmodel);
		// vbo组织方法，组织为GltfPrimMesh
		void initRayTracingScene(void* verticesData, void* indicesData);
		void createMaterialBuffer(VkCommandBuffer cmdBuf, const bgfx::GltfScene& gltf);
		void createLightBuffer(VkCommandBuffer cmdBuf, const bgfx::GltfScene& gltf);
		void createVertexBuffer(VkCommandBuffer cmdBuf, const bgfx::GltfScene& gltf);
		void createInstanceDataBuffer(VkCommandBuffer cmdBuf, bgfx::GltfScene& gltf);

		void destroy();
		void updateCamera(const VkCommandBuffer& cmdBuf, float aspectRatio);

		VkDescriptorSetLayout            getDescLayout() { return m_descSetLayout; }
		VkDescriptorSet                  getDescSet() { return m_descSet; }

		bgfx::GltfScene& getScene() { return m_gltf; }
		const std::vector<bgfx::Buffer>& getBuffers(EBuffers b) { return m_buffers[b]; }

	private:
		void createTextureImages(VkCommandBuffer cmdBuf, tinygltf::Model& gltfModel);
		void createDescriptorSet(const bgfx::GltfScene& gltf);

		bgfx::GltfScene m_gltf;
		bgfx::GltfStats m_stats;

		std::string m_sceneName;
		SceneCamera m_camera{};

		// Setup
		bgfx::ResourceAllocator* m_pAlloc;  // Allocator for buffer, images, acceleration structures
		bgfx::DebugUtil          m_debug;   // Utility to name objects
		VkDevice                 m_device;
		uint32_t                 m_queueFamilyIndex{ 0 };
		VkQueue					 m_queue;

		// Resources
		std::array<bgfx::Buffer, 5>                            m_buffer;           // For single buffer
		std::array<std::vector<bgfx::Buffer>, 2>               m_buffers;          // For array of buffers (vertex/index)
		std::vector<bgfx::Texture>                             m_textures;         // vector of all textures of the scene
		std::vector<std::pair<bgfx::Image, VkImageCreateInfo>> m_images;           // vector of all images of the scene
		std::vector<size_t>                                    m_defaultTextures;  // for cleanup

		VkDescriptorPool      m_descPool{ VK_NULL_HANDLE };
		VkDescriptorSetLayout m_descSetLayout{ VK_NULL_HANDLE };
		VkDescriptorSet       m_descSet{ VK_NULL_HANDLE };

		void importDrawableNodes(bgfx::VertexBufferHandle vbh, bgfx::IndexBufferHandle ibh);
	};
}
