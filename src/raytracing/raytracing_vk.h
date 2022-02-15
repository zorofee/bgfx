#pragma once

#include <map>
#include "vulkan-local/vulkan_core.h"
#include <vulkan-local/vulkan.h>
#include "accelstruct.h"
#include "raytracing_scene.h"
#include "nvpro_core/nvmath/nvmath.h"
#include "raytracing_builder.h"
#include "functions_vk.h"
#include "queue.h"
#include "../renderer_vk.h"
#include "rtx_pipeline.h"
#include "render_output.h"

#define ALLOC_DMA  // <--- This is in the CMakeLists.txt
#include "nvpro_core/nvvk/resource_allocator.h"
#if defined(ALLOC_DMA)
#include "nvpro_core/nvvk/memallocator_dma_vk.h"
typedef bgfx::ResourceAllocatorDma Allocator;
#endif

namespace bgfx{

	enum ERtShaderStages
	{
		eRaygen,
		eMiss,
		eMiss2,
		eClosestHit,
		eShaderGroupCount
	};

	struct VkRayTracingCreateInfo {
		VkInstance instance{};
		VkDevice	device{};
		VkPhysicalDevice physicalDevice{};
		std::vector<uint32_t> queueFamilyIndices{};
		std::vector<uint32_t> queueIndices{};
		VkSurfaceKHR  surface{};
		VkSwapchainKHR swapchain{};
		VkFramebuffer framebuffers{};
		VkExtent2D  viewSize{};
	};


	using vec2 = nvmath::vec2f;
	using vec3 = nvmath::vec3f;
	using vec4 = nvmath::vec4f;
	using mat4 = nvmath::mat4f;
	using uint = unsigned int;
	// Push constant structure for the ray tracer
	struct PushConstantRay
	{
		vec4  clearColor;
		vec3  lightPosition;
		float lightIntensity;
		int   lightType;
	};


	class RayTracingBase {


	};

	class RayTracingVK : public RayTracingBase{
	public:

		enum EQueues
		{
			eGCT0,
			eGCT1,
			eCompute,
			eTransfer
		};

		void setListOfFunctions(std::map<EVkFunctionName,void*>& funcMap);
		// const VkInstance& instance, const VkDevice& device, const VkPhysicalDevice& physicalDevice, uint32_t graphicsQueueIndex

		void setup(const VkRayTracingCreateInfo& info);

		void createRenderPass();

		void createFrameBuffers();

		void createCommandBuffers();

		void createDepthBuffer();

		void createDescriptorSetLayout();

		void initRayTracingScene(const char* filename);

		void initRayTracingScene(void* verticesData, void* indicesData);

		void createAccelerationStructure();

		void createRender();

		void createOffscreenRender();

		//-------update---------
		void render(VkFramebuffer fbo, uint32_t curFrame=0);

	private:
		void updateUniformBuffer(const VkCommandBuffer& cmdBuf);
		void renderScene(const VkCommandBuffer& cmdBuf);
		void drawPost(const VkCommandBuffer& cmdBuf);
		void submitFrame();
		uint32_t getMemoryType(uint32_t typeBits, const VkMemoryPropertyFlags& properties) const;

	protected:
		VkInstance		 m_instance{};
		VkDevice		 m_device{};
		VkSurfaceKHR	 m_surface{};
		VkPhysicalDevice m_physicalDevice{};
		VkCommandPool	 m_cmdPool{ VK_NULL_HANDLE };
		uint32_t         m_queueFamilyIndex{ VK_QUEUE_FAMILY_IGNORED };
		std::vector<bgfx::Queue> m_queues;
		VkQueue          m_queue{ VK_NULL_HANDLE };

		// #Drawing/Surface
		VkPipelineCache              m_pipelineCache{ VK_NULL_HANDLE };  // Cache for pipeline/shaders

		// #VKRay
		std::vector<VkRayTracingShaderGroupCreateInfoKHR>   m_rtShaderGroups;
		VkDescriptorSetLayout								m_rtDescSetLayout;
		VkPipelineLayout                                    m_rtPipelineLayout;
		VkPipeline											m_rtPipeline;

		VkDescriptorPool            m_descPool{ VK_NULL_HANDLE };
		VkDescriptorSetLayout		m_descSetLayout;
		VkDescriptorSet             m_descSet{ VK_NULL_HANDLE };
		DescriptorSetBindings		m_bind;
	public:
		VkPhysicalDeviceRayTracingPipelinePropertiesKHR m_rtProperties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR };
		RayTracingScene		m_scene;
		RayTracingBuilder	m_rtBuilder;
		AccelStructure		m_accelStruct;
		Allocator			m_alloc;  // Allocator for buffer, images, acceleration structures
		DebugUtil			m_debug;  // Utility to name objects
		Renderer*			m_render;
		RenderOutput		m_offscreen;
		VkImage                      m_depthImage{ VK_NULL_HANDLE };     // Depth/Stencil
		VkDeviceMemory               m_depthMemory{ VK_NULL_HANDLE };    // Depth/Stencil
		VkImageView                  m_depthView{ VK_NULL_HANDLE };      // Depth/Stencil
			  //bgfx::DebugUtil		m_debug;  // Utility to name objects
		VkExtent2D                   m_size{ 0, 0 };
		VkRenderPass                 m_renderPass{ VK_NULL_HANDLE };     // Base render pass
		std::vector<VkFramebuffer>   m_framebuffers;                   // All framebuffers, correspond to the Swapchain
		std::vector<VkCommandBuffer> m_commandBuffers;                 // Command buffer per nb element in Swapchain
		VkRect2D m_renderRegion{};

		// Surface buffer formats
		VkFormat m_colorFormat{ VK_FORMAT_B8G8R8A8_UNORM };
		VkFormat m_depthFormat{ VK_FORMAT_UNDEFINED };

		Buffer m_sunAndSkyBuffer;

	};

}
