#pragma once
#include <iostream>
#include <assert.h>
#include <array>
#include "raytracing_vk.h"
#include "rtx_pipeline.h"

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
		m_queueFamilyIndex = info.queueFamilyIndices[0];

		for (size_t i = 0; i < info.queueIndices.size(); i++)
		{
			bgfx::Queue rtQueue;
			rtQueue.familyIndex = m_queueFamilyIndex;
			rtQueue.queueIndex = info.queueIndices[i];
			BGFX_VKAPI(vkGetDeviceQueue)(m_device, rtQueue.familyIndex, rtQueue.queueIndex, &rtQueue.queue);
			m_queues.push_back(rtQueue);
		}

		m_alloc.init(info.instance, info.device, info.physicalDevice);
		m_accelStruct.setup(m_device, info.physicalDevice, m_queues[eCompute].familyIndex, &m_alloc);
		m_scene.setup(m_device, info.physicalDevice, m_queues[eGCT1].familyIndex, m_queues[eGCT1].queue, &m_alloc);

		m_render = new RtxPipeline;
		int testfamilyindex = 0;
		m_render->setup(m_device,m_physicalDevice, m_queues[eTransfer].familyIndex,&m_alloc);
	}
	//--------------------------------------------------------------------------------------------------
	// Loading the scene file, setting up all scene buffers, create the acceleration structures
	// for the loaded models.
	// Reorganize tinyGLtf::Model data into struct GltfScene.
	//
	void RayTracingVK::initRayTracingScene(const char* filename)
	{
		std::string fn = filename;
		m_scene.load(fn);
		//m_accelStruct.create(m_scene.getScene(), m_scene.getBuffers(RayTracingScene::eVertex), m_scene.getBuffers(RayTracingScene::eIndex));
	}
	//--------------------------------------------------------------------------------------------------
	void RayTracingVK::initRayTracingScene(void* verticesData, void* indicesData)
	{
		m_scene.initRayTracingScene(verticesData, indicesData);
	}
	//--------------------------------------------------------------------------------------------------
	void RayTracingVK::createAccelerationStructure()
	{
		// m_scene需要把场景组织到自己的gltfScene
		m_accelStruct.create(m_scene.getScene(), m_scene.getBuffers(RayTracingScene::eVertex), m_scene.getBuffers(RayTracingScene::eIndex));
	}
	//--------------------------------------------------------------------------------------------------
	void RayTracingVK::createRender()
	{
		m_render->create(m_size, { m_accelStruct.getDescLayout()});
	}
	//--------------------------------------------------------------------------------------------------

}
