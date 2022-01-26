#pragma once

#include <map>
#include <vulkan-local/vulkan.h>
#include "nvmath.h"
#include "raytracing_builder.h"
#include "functions_vk.h"
#include "../renderer_vk.h"

#define BGFX_VKAPI(func) (*(PFN_##func*)FunctionMapVk::Get()->getFunction(EVkFunctionName::func))
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
		void setListOfFunctions(std::map<EVkFunctionName,void*>& funcMap);

		void setup(const VkRayTracingCreateInfo& info);

		void setSwapChain();

		void initRayTracing();

		void createBottomLevelAS();

		void reateTopLevelAS();

		void createRtDescriptorSet();

		void createRtPipeline(const bgfx::vk::ProgramVK& _program);

		void createRtShaderBindingTable();

	protected:
		VkInstance		 m_instance{};
		VkDevice		 m_device{};
		VkSurfaceKHR	 m_surface{};
		VkPhysicalDevice m_physicalDevice{};
		VkQueue          m_queue{ VK_NULL_HANDLE };
		VkCommandPool	 m_cmdPool{ VK_NULL_HANDLE };
		uint32_t         m_graphicsQueueIndex{ VK_QUEUE_FAMILY_IGNORED };

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
		RayTracingBuilder m_rtBuilder;


	};

}
