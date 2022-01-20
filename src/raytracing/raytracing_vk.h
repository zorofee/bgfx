#pragma once

#include <map>
#include <vulkan-local/vulkan.h>
#include "raytracing_builder.h"
#include "functions_vk.h"

#define BGFX_VKAPI(func) (*(PFN_##func*)FunctionMapVk::Get()->getFunction(EVkFunctionName::func))
namespace bgfx{

	class RayTracingBase {


	};

	class RayTracingVK : public RayTracingBase{
	public:
		void setListOfFunctions(std::map<EVkFunctionName,void*>& funcMap);

		void setup(const VkInstance& instance, const VkDevice& device, const VkPhysicalDevice& physicalDevice, uint32_t graphicsQueueIndex);

		void setSwapChain();

		void initDevice();

		void initRayTracing();

		void createBottomLevelAS();

		void reateTopLevelAS();

		void createRtDescriptorSet();

		void createRtPipeline();

		void createRtShaderBindingTable();

	protected:
		VkInstance		 m_instance{};
		VkDevice		 m_device{};
		VkSurfaceKHR	 m_surface{};
		VkPhysicalDevice m_physicalDevice{};
		VkQueue          m_queue{ VK_NULL_HANDLE };
		VkCommandPool	 m_cmdPool{ VK_NULL_HANDLE };
		uint32_t         m_graphicsQueueIndex{ VK_QUEUE_FAMILY_IGNORED };

		//Drawing/Surface
		VkPipelineCache              m_pipelineCache{ VK_NULL_HANDLE };  // Cache for pipeline/shaders

	public:
		RayTracingBuilder m_rtBuilder;


	};

}
