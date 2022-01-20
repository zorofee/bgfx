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


	}

	void initDevice() {

	}


	void RayTracingVK::initRayTracing(){

	}

	void RayTracingVK::createBottomLevelAS(){

	}


}
