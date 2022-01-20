#pragma once
#include <iostream>
#include <assert.h>
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

	void RayTracingVK::setup(const VkInstance& instance, const VkDevice& device, const VkPhysicalDevice& physicalDevice, uint32_t graphicsQueueIndex) {
		std::cout << "hello raytracing!";

		m_instance = instance;
		m_device = device;
		m_physicalDevice = physicalDevice;
		m_graphicsQueueIndex = graphicsQueueIndex;

			
		PFN_vkGetDeviceQueue* vkGetDeviceQueue = (PFN_vkGetDeviceQueue*)FunctionMapVk::Get()->getFunction(EVkFunctionName::vkGetDeviceQueue);
		PFN_vkCreateCommandPool* vkCreateCommandPool = (PFN_vkCreateCommandPool*)FunctionMapVk::Get()->getFunction(EVkFunctionName::vkCreateCommandPool);
		PFN_vkCreatePipelineCache* vkCreatePipelineCache = (PFN_vkCreatePipelineCache*)FunctionMapVk::Get()->getFunction(EVkFunctionName::vkCreatePipelineCache);
		PFN_vkEnumeratePhysicalDevices* vkEnumeratePhysicalDevices = (PFN_vkEnumeratePhysicalDevices*)FunctionMapVk::Get()->getFunction(EVkFunctionName::vkEnumeratePhysicalDevices);

		uint32_t deviceCount = 0;
		(*vkEnumeratePhysicalDevices)(m_instance, &deviceCount, nullptr);
		if (deviceCount == 0)
		{
			throw std::runtime_error("failed to find GPUs");
		}
		std::vector<VkPhysicalDevice> devices(deviceCount);
		(*vkEnumeratePhysicalDevices)(m_instance, &deviceCount, devices.data());


		VkResult result = VK_SUCCESS;
		(*vkGetDeviceQueue)(m_device, m_graphicsQueueIndex, 0, &m_queue);

		VkCommandPoolCreateInfo poolCreateInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
		poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		result = (*vkCreateCommandPool)(m_device, &poolCreateInfo, nullptr, &m_cmdPool);

		VkPipelineCacheCreateInfo pipelineCacheInfo{ VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO };
		result = (*vkCreatePipelineCache)(m_device, &pipelineCacheInfo, nullptr, &m_pipelineCache);


	}

}
