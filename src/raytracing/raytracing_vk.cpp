#pragma once
#include <iostream>
#include <assert.h>
#include <array>
#include "raytracing_vk.h"

namespace bgfx {

	FunctionMapVk* FunctionMapVk::m_instance = nullptr;

	FunctionMapVk* FunctionMapVk::Get(){
		if (m_instance == nullptr){
			m_instance = new FunctionMapVk();
		}
		return m_instance;
	}

	void FunctionMapVk::setup(std::map<EVkFunctionName, void*>& funcMap){
		m_funcMap = funcMap;
	}

	void* FunctionMapVk::getFunction(EVkFunctionName funcName)
	{
		assert(m_funcMap.find(funcName)!= m_funcMap.end());
		return m_funcMap[funcName];
	}

	void RayTracingVK::setListOfFunctions(std::map<EVkFunctionName, void*>& funcMap)
	{
		FunctionMapVk::Get()->setup(funcMap);
	}

	void RayTracingVK::setup(const VkRayTracingCreateInfo& info)
	{
		m_instance = info.instance;
		m_device = info.device;
		m_physicalDevice = info.physicalDevice;
		m_graphicsQueueIndex = info.queueIndices[0];


		uint32_t deviceCount = 0;
		BGFX_VKAPI(vkEnumeratePhysicalDevices)(m_instance, &deviceCount, nullptr);
		if (deviceCount == 0)
		{
			throw std::runtime_error("failed to find GPUs");
		}
		std::vector<VkPhysicalDevice> devices(deviceCount);
		BGFX_VKAPI(vkEnumeratePhysicalDevices)(m_instance, &deviceCount, devices.data());


		VkResult result = VK_SUCCESS;
		BGFX_VKAPI(vkGetDeviceQueue)(m_device, m_graphicsQueueIndex, 0, &m_queue);

		VkCommandPoolCreateInfo poolCreateInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
		poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		BGFX_VKAPI(vkCreateCommandPool)(m_device, &poolCreateInfo, nullptr, &m_cmdPool);

		VkPipelineCacheCreateInfo pipelineCacheInfo{ VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO };
		BGFX_VKAPI(vkCreatePipelineCache)(m_device, &pipelineCacheInfo, nullptr, &m_pipelineCache);
		////--------------------------------------------------------------------------------------------------
		// Memory allocator for buffers and images
		m_alloc.init(info.instance, info.device, info.physicalDevice);
		// m_debug.setup(m_device);
		// Compute queues can be use for acceleration structures
		// 暂时用info.queueIndices[0] + 2
		m_accelStruct.setup(m_device, info.physicalDevice, info.queueIndices[0] + 2, &m_alloc);

	}
	//--------------------------------------------------------------------------------------------------
	void RayTracingVK::setup(const VkInstance& instance,
		const VkDevice& device,
		const VkPhysicalDevice& physicalDevice,
		uint32_t graphicsQueueIndex,
		uint32_t computQueueIndex)
	{
		std::cout << "hello raytracing!";

		//--------------------------------------------------------------------------------------------------
		// AppBaseVk::setup
		m_instance = instance;
		m_device = device;
		m_physicalDevice = physicalDevice;
		m_graphicsQueueIndex = graphicsQueueIndex;

		uint32_t deviceCount = 0;
		BGFX_VKAPI(vkEnumeratePhysicalDevices)(m_instance, &deviceCount, nullptr);
		if (deviceCount == 0)
		{
			throw std::runtime_error("failed to find GPUs");
		}
		std::vector<VkPhysicalDevice> devices(deviceCount);
		BGFX_VKAPI(vkEnumeratePhysicalDevices)(m_instance, &deviceCount, devices.data());

		VkResult result = VK_SUCCESS;
		BGFX_VKAPI(vkGetDeviceQueue)(m_device, m_graphicsQueueIndex, 0, &m_queue);

		VkCommandPoolCreateInfo poolCreateInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
		poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		BGFX_VKAPI(vkCreateCommandPool)(m_device, &poolCreateInfo, nullptr, &m_cmdPool);

		VkPipelineCacheCreateInfo pipelineCacheInfo{ VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO };
		BGFX_VKAPI(vkCreatePipelineCache)(m_device, &pipelineCacheInfo, nullptr, &m_pipelineCache);
		////--------------------------------------------------------------------------------------------------
		// Memory allocator for buffers and images
		m_alloc.init(instance, device, physicalDevice);
		//m_debug.setup(m_device);
		// Compute queues can be use for acceleration structures
		m_accelStruct.setup(m_device, physicalDevice, computQueueIndex, &m_alloc);

	}
	//--------------------------------------------------------------------------------------------------
	void RayTracingVK::initRayTracing() {
		VkPhysicalDeviceProperties2 prop2{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
		prop2.pNext = &m_rtProperties;
		BGFX_VKAPI(vkGetPhysicalDeviceProperties2)(m_physicalDevice, &prop2);

		//m_rtBuilder.setup(m_device,nullptr,m_graphicsQueueIndex);
	}
	//--------------------------------------------------------------------------------------------------
	void RayTracingVK::createBottomLevelAS()
	{

	}
	//--------------------------------------------------------------------------------------------------
	void RayTracingVK::createRtPipeline(const  bgfx::vk::ProgramVK& _program)
	{

		// All stages
		std::array<VkPipelineShaderStageCreateInfo, eShaderGroupCount> stages{};
		VkPipelineShaderStageCreateInfo stage{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		stage.pName = "main";  // All the same entry point
		
		// Raygen
		stage.module = _program.m_rayGen->m_module;// (shaderStages[eRaygen]);
		stage.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
		stages[eRaygen] = stage;
	
		// Miss
		stage.module = _program.m_miss->m_module;// nvvk::createShaderModule(m_device, nvh::loadFile("spv/raytrace.rmiss.spv", true, defaultSearchPaths, true));
		stage.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
		stages[eMiss] = stage;
	
		// The second miss shader is invoked when a shadow ray misses the geometry. It simply indicates that no occlusion has been found
		stage.module = _program.m_miss2->m_module;
		stage.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
		stages[eMiss2] = stage;
		// Hit Group - Closest Hit
		stage.module = _program.m_closestHit->m_module;
		stage.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
		stages[eClosestHit] = stage;
		

		// Shader groups
		VkRayTracingShaderGroupCreateInfoKHR group{ VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR };
		group.anyHitShader = VK_SHADER_UNUSED_KHR;
		group.closestHitShader = VK_SHADER_UNUSED_KHR;
		group.generalShader = VK_SHADER_UNUSED_KHR;
		group.intersectionShader = VK_SHADER_UNUSED_KHR;
	
		// Raygen
		group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		group.generalShader = eRaygen;
		m_rtShaderGroups.push_back(group);

		// Miss
		group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		group.generalShader = eMiss;
		m_rtShaderGroups.push_back(group);

		// Shadow Miss
		group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		group.generalShader = eMiss2;
		m_rtShaderGroups.push_back(group);

		// closest hit shader
		group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
		group.generalShader = VK_SHADER_UNUSED_KHR;
		group.closestHitShader = eClosestHit;
		m_rtShaderGroups.push_back(group);

		// Push constant: we want to be able to update constants used by the shaders
		VkPushConstantRange pushConstant{ VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR,
										 0, sizeof(PushConstantRay) };

		
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
		pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
		pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstant;

		// Descriptor sets: one specific to ray tracing, and one shared with the rasterization pipeline
		std::vector<VkDescriptorSetLayout> rtDescSetLayouts = { m_rtDescSetLayout, m_descSetLayout };
		pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(rtDescSetLayouts.size());
		pipelineLayoutCreateInfo.pSetLayouts = rtDescSetLayouts.data();

		BGFX_VKAPI(vkCreatePipelineLayout)(m_device, &pipelineLayoutCreateInfo, nullptr, &m_rtPipelineLayout);


		// Assemble the shader stages and recursion depth info into the ray tracing pipeline
		VkRayTracingPipelineCreateInfoKHR rayPipelineInfo{ VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR };
		rayPipelineInfo.stageCount = static_cast<uint32_t>(stages.size());  // Stages are shaders
		rayPipelineInfo.pStages = stages.data();

		// In this case, m_rtShaderGroups.size() == 4: we have one raygen group,
		// two miss shader groups, and one hit group.
		rayPipelineInfo.groupCount = static_cast<uint32_t>(m_rtShaderGroups.size());
		rayPipelineInfo.pGroups = m_rtShaderGroups.data();

		// The ray tracing process can shoot rays from the camera, and a shadow ray can be shot from the
		// hit points of the camera rays, hence a recursion level of 2. This number should be kept as low
		// as possible for performance reasons. Even recursive ray tracing should be flattened into a loop
		// in the ray generation to avoid deep recursion.
		rayPipelineInfo.maxPipelineRayRecursionDepth = 2;  // Ray depth
		rayPipelineInfo.layout = m_rtPipelineLayout;

		BGFX_VKAPI(vkCreateRayTracingPipelinesKHR)(m_device, {}, {}, 1, &rayPipelineInfo, nullptr, &m_rtPipeline);


		// Spec only guarantees 1 level of "recursion". Check for that sad possibility here.
		if (m_rtProperties.maxRayRecursionDepth <= 1)
		{
			throw std::runtime_error("Device fails to support ray recursion (m_rtProperties.maxRayRecursionDepth <= 1)");
		}

		for (auto& s : stages)
			BGFX_VKAPI(vkDestroyShaderModule)(m_device, s.module, nullptr);

		/**/
	}
	//--------------------------------------------------------------------------------------------------
	void RayTracingVK::createAccelerationStructure(bgfx::GltfScene& gltfScene, const std::vector<bgfx::Buffer>& vertex, const std::vector<bgfx::Buffer>& index)
	{
		//m_accelStruct.create(m_scene.getScene(), vertex, index);
	}
}
