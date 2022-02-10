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
		VkExtent2D  sie{};
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

		void setSwapChain();

		void initRayTracingScene(const char* filename);

		void initRayTracingScene(void* verticesData, void* indicesData);

		void reateTopLevelAS();

		void createRtDescriptorSet();

		void createRtPipeline(const bgfx::vk::ProgramVK& _program);

		void createRtShaderBindingTable();

		void createAccelerationStructure();

		void createRender();

	protected:
		VkInstance		 m_instance{};
		VkDevice		 m_device{};
		VkSurfaceKHR	 m_surface{};
		VkPhysicalDevice m_physicalDevice{};
		VkCommandPool	 m_cmdPool{ VK_NULL_HANDLE };
		uint32_t         m_queueFamilyIndex{ VK_QUEUE_FAMILY_IGNORED };
		std::vector<bgfx::Queue> m_queues;

		// #Drawing/Surface
		VkPipelineCache              m_pipelineCache{ VK_NULL_HANDLE };  // Cache for pipeline/shaders

		// #VKRay
		std::vector<VkRayTracingShaderGroupCreateInfoKHR>   m_rtShaderGroups;
		VkDescriptorSetLayout								m_rtDescSetLayout;
		VkDescriptorSetLayout								m_descSetLayout;
		VkPipelineLayout                                    m_rtPipelineLayout;
		VkPipeline											m_rtPipeline;

	public:
		VkPhysicalDeviceRayTracingPipelinePropertiesKHR m_rtProperties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR };
		RayTracingScene		m_scene;
		RayTracingBuilder	m_rtBuilder;
		AccelStructure		m_accelStruct;
		Allocator			m_alloc;  // Allocator for buffer, images, acceleration structures
		//bgfx::DebugUtil		m_debug;  // Utility to name objects
		VkExtent2D                   m_size{ 0, 0 };

		Renderer* m_render;
	};

}
