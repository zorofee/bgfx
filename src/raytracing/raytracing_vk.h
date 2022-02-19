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
#include "hdr_sampling.h"

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
		VkInstance				instance{};
		VkDevice				device{};
		VkPhysicalDevice		physicalDevice{};
		std::vector<uint32_t>	queueFamilyIndices{};
		std::vector<uint32_t>	queueIndices{};
		VkSurfaceKHR			surface{};
		VkSwapchainKHR			swapchain{};
		VkFramebuffer			framebuffers{};
		VkExtent2D				viewSize{};
	};


	using vec2 = nvmath::vec2f;
	using vec3 = nvmath::vec3f;
	using vec4 = nvmath::vec4f;
	using mat4 = nvmath::mat4f;
	using uint = unsigned int;
	
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

		void setup(const VkRayTracingCreateInfo& info);

		void createRenderPass();

		void createDepthBuffer();

		void createDescriptorSetLayout();

		void initRayTracingScene(const char* filename);

		void initRayTracingScene(void* verticesData, void* indicesData);

		void createAccelerationStructure();

		void createRender();

		void createOffscreenRender();

		void loadEnvironmentHdr(const std::string& hdrFilename);

		void createCommandPool(uint32_t _queueFamilyIndex);

		void createSyncObjects(uint32_t _maxframe, uint32_t _imagesize);

		void createRenderPass(VkFormat swapChainImageFormat);

		void addFramebuffer(VkFramebuffer _fbo);

		void createCommandBuffers();

		void setRenderRegion(const VkRect2D& size);

		void resetFrame();

		//-------update---------
		void updateFrame();

		void drawFrame(VkQueue graphicsQueue, uint32_t currentFrame, uint32_t imageIndex);
	private:
		void updateUniformBuffer(const VkCommandBuffer& cmdBuf);

		void renderScene(const VkCommandBuffer& cmdBuf);

		void drawPost(const VkCommandBuffer& cmdBuf);

		void submit(VkCommandBuffer cmdbuff, VkQueue graphicsQueue, uint32_t currentFrame, uint32_t imageIndex);

		uint32_t getMemoryType(uint32_t typeBits, const VkMemoryPropertyFlags& properties) const;

	protected:
		VkInstance						m_instance{};
		VkDevice						m_device{};
		VkPhysicalDevice				m_physicalDevice{};

		uint32_t						m_queueFamilyIndex{ VK_QUEUE_FAMILY_IGNORED };
		std::vector<bgfx::Queue>		m_queues;
		VkQueue							m_queue{ VK_NULL_HANDLE };

		VkCommandPool					m_cmdPool;
		VkRenderPass					m_renderPass{ VK_NULL_HANDLE };     // Base render pass
		std::vector<VkFramebuffer>		m_framebuffers;                   // All framebuffers, correspond to the Swapchain
		std::vector<VkCommandBuffer>	m_commandBuffers;                 // Command buffer per nb element in Swapchain

		VkDescriptorPool				m_descPool{ VK_NULL_HANDLE };
		VkDescriptorSetLayout			m_descSetLayout;
		VkDescriptorSet					m_descSet{ VK_NULL_HANDLE };
		DescriptorSetBindings			m_bind;

		RayTracingScene		m_scene;
		AccelStructure		m_accelStruct;
		Allocator			m_alloc;  // Allocator for buffer, images, acceleration structures
		DebugUtil			m_debug;  // Utility to name objects
		Renderer*			m_render;
		RenderOutput		m_offscreen;
		HdrSampling         m_skydome;
		Buffer				m_sunAndSkyBuffer;

		VkImage                      m_depthImage{ VK_NULL_HANDLE };     // Depth/Stencil
		VkDeviceMemory               m_depthMemory{ VK_NULL_HANDLE };    // Depth/Stencil
		VkImageView                  m_depthView{ VK_NULL_HANDLE };      // Depth/Stencil

		VkExtent2D                   m_size{ 0, 0 };
		VkRect2D					 m_renderRegion{};

		std::vector<VkSemaphore> imageAvailableSemaphores;
		std::vector<VkSemaphore> renderFinishedSemaphores;
		std::vector<VkFence>	 inFlightFences;
		std::vector<VkFence>	 imagesInFlight;

		// Surface buffer formats
		VkFormat m_colorFormat{ VK_FORMAT_B8G8R8A8_UNORM };
		VkFormat m_depthFormat{ VK_FORMAT_UNDEFINED };

		int      m_maxFrames{ 100000 };

		bool	 isBegin = false;

		RtxState m_rtxState{
				0,       // frame;
				10,      // maxDepth;
				1,       // maxSamples;
				1,       // fireflyClampThreshold;
				1,       // hdrMultiplier;
				0,       // debugging_mode;
				0,       // pbrMode;
				0,       // _pad0;
				{0, 0},  // size;
				0,       // minHeatmap;
				65000    // maxHeatmap;
		};
	};

}
