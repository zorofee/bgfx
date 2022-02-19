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

		m_size = VkExtent2D{ 1280,720 };

		BGFX_VKAPI(vkGetDeviceQueue)(m_device, m_queueFamilyIndex, 0, &m_queue);

		for (size_t i = 0; i < info.queueIndices.size(); i++)
		{
			bgfx::Queue rtQueue;
			rtQueue.familyIndex = m_queueFamilyIndex;
			rtQueue.queueIndex = info.queueIndices[i];
			BGFX_VKAPI(vkGetDeviceQueue)(m_device, rtQueue.familyIndex, rtQueue.queueIndex, &rtQueue.queue);
			m_queues.push_back(rtQueue);
		}

		m_alloc.init(info.instance, info.device, info.physicalDevice);
		m_debug.setup(m_device);

		m_accelStruct.setup(m_device, info.physicalDevice, m_queues[eCompute].familyIndex, &m_alloc);
		m_scene.setup(m_device, info.physicalDevice, m_queues[eGCT1].familyIndex, m_queues[eGCT1].queue, &m_alloc);

		m_offscreen.setup(m_device, info.physicalDevice, m_queues[eTransfer].familyIndex, &m_alloc);
		m_skydome.setup(m_device, info.physicalDevice, m_queues[eTransfer].familyIndex, &m_alloc);


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
		m_render->create(m_size, { m_accelStruct.getDescLayout(), m_offscreen.getDescLayout(), m_scene.getDescLayout(), m_descSetLayout });

		isBegin = true;
	}
	//--------------------------------------------------------------------------------------------------
	void RayTracingVK::createRenderPass()
	{
		if (m_renderPass)
		{
			BGFX_VKAPI(vkDestroyRenderPass)(m_device, m_renderPass, nullptr);
		}

		std::array<VkAttachmentDescription, 2> attachments{};
		// Color attachment
		attachments[0].format = m_colorFormat;
		attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;

		// Depth attachment
		/*
		attachments[1].format = m_depthFormat;
		attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
		*/
		// One color, one depth
		const VkAttachmentReference colorReference{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
		const VkAttachmentReference depthReference{ 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

		std::array<VkSubpassDependency, 1> subpassDependencies{};
		// Transition from final to initial (VK_SUBPASS_EXTERNAL refers to all commands executed outside of the actual renderpass)
		subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		subpassDependencies[0].dstSubpass = 0;
		subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		subpassDependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		VkSubpassDescription subpassDescription{};
		subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescription.colorAttachmentCount = 1;
		subpassDescription.pColorAttachments = &colorReference;
		//subpassDescription.pDepthStencilAttachment = &depthReference;

		VkRenderPassCreateInfo renderPassInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
		renderPassInfo.attachmentCount = 1;//static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpassDescription;
		renderPassInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
		renderPassInfo.pDependencies = subpassDependencies.data();

		BGFX_VKAPI(vkCreateRenderPass)(m_device, &renderPassInfo, nullptr, &m_renderPass);

#ifdef _DEBUG
		VkDebugUtilsObjectNameInfoEXT nameInfo{ VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
		nameInfo.objectHandle = (uint64_t)m_renderPass;
		nameInfo.objectType = VK_OBJECT_TYPE_RENDER_PASS;
		nameInfo.pObjectName = R"(AppBaseVk)";
		BGFX_VKAPI(vkSetDebugUtilsObjectNameEXT)(m_device, &nameInfo);
#endif  // _DEBUG
	}
	//--------------------------------------------------------------------------------------------------
	void RayTracingVK::createOffscreenRender()
	{
		m_offscreen.create(m_size, m_renderPass);
	}
	//--------------------------------------------------------------------------------------------------
	void RayTracingVK::loadEnvironmentHdr(const std::string& hdrFilename)
	{
		m_skydome.loadEnvironment(hdrFilename);
	}
	//--------------------------------------------------------------------------------------------------
	void RayTracingVK::createDepthBuffer()
	{
		if (m_depthView)
			BGFX_VKAPI(vkDestroyImageView)(m_device, m_depthView, nullptr);
		if (m_depthImage)
			BGFX_VKAPI(vkDestroyImage)(m_device, m_depthImage, nullptr);
		if (m_depthMemory)
			BGFX_VKAPI(vkFreeMemory)(m_device, m_depthMemory, nullptr);

		// Depth information
		const VkImageAspectFlags aspect = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		VkImageCreateInfo        depthStencilCreateInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
		depthStencilCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		depthStencilCreateInfo.extent = VkExtent3D{ m_size.width, m_size.height, 1 };
		depthStencilCreateInfo.format = VK_FORMAT_D24_UNORM_S8_UINT;//m_depthFormat;
		depthStencilCreateInfo.mipLevels = 1;
		depthStencilCreateInfo.arrayLayers = 1;
		depthStencilCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		depthStencilCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		// Create the depth image
		BGFX_VKAPI(vkCreateImage)(m_device, &depthStencilCreateInfo, nullptr, &m_depthImage);

#ifdef _DEBUG
		std::string                   name = std::string("AppBaseDepth");
		VkDebugUtilsObjectNameInfoEXT nameInfo{ VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
		nameInfo.objectHandle = (uint64_t)m_depthImage;
		nameInfo.objectType = VK_OBJECT_TYPE_IMAGE;
		nameInfo.pObjectName = R"(AppBase)";
		BGFX_VKAPI(vkSetDebugUtilsObjectNameEXT)(m_device, &nameInfo);
#endif  // _DEBUG

		// Allocate the memory
		VkMemoryRequirements memReqs;
		BGFX_VKAPI(vkGetImageMemoryRequirements)(m_device, m_depthImage, &memReqs);
		VkMemoryAllocateInfo memAllocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
		memAllocInfo.allocationSize = memReqs.size;
		memAllocInfo.memoryTypeIndex = getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		BGFX_VKAPI(vkAllocateMemory)(m_device, &memAllocInfo, nullptr, &m_depthMemory);

		// Bind image and memory
		BGFX_VKAPI(vkBindImageMemory)(m_device, m_depthImage, m_depthMemory, 0);

		// Create an image barrier to change the layout from undefined to DepthStencilAttachmentOptimal
		VkCommandBufferAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		allocateInfo.commandBufferCount = 1;
		allocateInfo.commandPool = m_cmdPool;  //todo
		allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		VkCommandBuffer cmdBuffer;
		BGFX_VKAPI(vkAllocateCommandBuffers)(m_device, &allocateInfo, &cmdBuffer);

		VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		BGFX_VKAPI(vkBeginCommandBuffer)(cmdBuffer, &beginInfo);


		// Put barrier on top, Put barrier inside setup command buffer
		VkImageSubresourceRange subresourceRange{};
		subresourceRange.aspectMask = aspect;
		subresourceRange.levelCount = 1;
		subresourceRange.layerCount = 1;
		VkImageMemoryBarrier imageMemoryBarrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		imageMemoryBarrier.image = m_depthImage;
		imageMemoryBarrier.subresourceRange = subresourceRange;
		imageMemoryBarrier.srcAccessMask = VkAccessFlags();
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		const VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		const VkPipelineStageFlags destStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

		BGFX_VKAPI(vkCmdPipelineBarrier)(cmdBuffer, srcStageMask, destStageMask, VK_FALSE, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
		BGFX_VKAPI(vkEndCommandBuffer)(cmdBuffer);

		VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cmdBuffer;
		BGFX_VKAPI(vkQueueSubmit)(m_queue, 1, &submitInfo, {});
		BGFX_VKAPI(vkQueueWaitIdle)(m_queue);
		BGFX_VKAPI(vkFreeCommandBuffers)(m_device, m_cmdPool, 1, &cmdBuffer);  //todo

		// Setting up the view
		VkImageViewCreateInfo depthStencilView{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
		depthStencilView.format = m_depthFormat;
		depthStencilView.subresourceRange = subresourceRange;
		depthStencilView.image = m_depthImage;
		BGFX_VKAPI(vkCreateImageView)(m_device, &depthStencilView, nullptr, &m_depthView);
	}
	//--------------------------------------------------------------------------------------------------
	void RayTracingVK::createDescriptorSetLayout()
	{
		VkShaderStageFlags flags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR
			| VK_SHADER_STAGE_ANY_HIT_BIT_KHR | VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;


		m_bind.addBinding({ EnvBindings::eSunSky, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_MISS_BIT_KHR | flags });
		m_bind.addBinding({ EnvBindings::eHdr, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, flags });  // HDR image
		m_bind.addBinding({ EnvBindings::eImpSamples, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, flags });   // importance sampling


		m_descPool = m_bind.createPool(m_device, 1);
		CREATE_NAMED_VK(m_descSetLayout, m_bind.createLayout(m_device));
		CREATE_NAMED_VK(m_descSet, allocateDescriptorSet(m_device, m_descPool, m_descSetLayout));

		// Using the environment
		std::vector<VkWriteDescriptorSet> writes;
		VkDescriptorBufferInfo            sunskyDesc{ m_sunAndSkyBuffer.buffer, 0, VK_WHOLE_SIZE };
		//[todo]
		VkDescriptorBufferInfo            accelImpSmpl{ m_skydome.m_accelImpSmpl.buffer, 0, VK_WHOLE_SIZE };
		writes.emplace_back(m_bind.makeWrite(m_descSet, EnvBindings::eSunSky, &sunskyDesc));
		writes.emplace_back(m_bind.makeWrite(m_descSet, EnvBindings::eHdr, &m_skydome.m_texHdr.descriptor));
		writes.emplace_back(m_bind.makeWrite(m_descSet, EnvBindings::eImpSamples, &accelImpSmpl));

		BGFX_VKAPI(vkUpdateDescriptorSets)(m_device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
	}

	//--------------------------------------------------------------------------------------------------
	void RayTracingVK::drawFrame(VkQueue graphicsQueue, uint32_t currentFrame, uint32_t imageIndex)
	{
		if (!isBegin)
			return;

		updateFrame();
		const VkCommandBuffer& cmdBuf = m_commandBuffers[currentFrame];

		VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		BGFX_VKAPI(vkBeginCommandBuffer)(cmdBuf, &beginInfo);


		updateUniformBuffer(cmdBuf);  // Updating UBOs

		// Rendering Scene (ray tracing)
		renderScene(cmdBuf);

		// Rendering pass in swapchain framebuffer + tone mapper, UI
		{
			std::array<VkClearValue, 2> clearValues;
			clearValues[0].color = { {0.0f, 0.0f, 0.0f, 0.0f} };
			clearValues[1].depthStencil = { 1.0f, 0 };

			VkRenderPassBeginInfo postRenderPassBeginInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
			postRenderPassBeginInfo.clearValueCount = 2;
			postRenderPassBeginInfo.pClearValues = clearValues.data();
			postRenderPassBeginInfo.renderPass = m_renderPass;
			postRenderPassBeginInfo.framebuffer = m_framebuffers[currentFrame];
			postRenderPassBeginInfo.renderArea = { {}, {800,600} };

			BGFX_VKAPI(vkCmdBeginRenderPass)(cmdBuf, &postRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);  //如果加上depth attachment就会报错

			// Draw the rendering result + tonemapper
			drawPost(cmdBuf);

			BGFX_VKAPI(vkCmdDraw)(cmdBuf, 3, 1, 0, 0);

			// Render the UI
			BGFX_VKAPI(vkCmdEndRenderPass)(cmdBuf);
		}

		// Submit for display
		BGFX_VKAPI(vkEndCommandBuffer)(cmdBuf);
		submit(cmdBuf,graphicsQueue, currentFrame, imageIndex);
	}
	//--------------------------------------------------------------------------------------------------
	void RayTracingVK::updateUniformBuffer(const VkCommandBuffer& cmdBuf)
	{
		//m_renderRegion = { 1080,694 };
		const float aspectRatio = m_renderRegion.extent.width / static_cast<float>(m_renderRegion.extent.height);

		m_scene.updateCamera(cmdBuf, aspectRatio);
	}
	//--------------------------------------------------------------------------------------------------
	void RayTracingVK::updateFrame()
	{
		if (m_rtxState.frame < m_maxFrames)
			m_rtxState.frame++;
	}
	//--------------------------------------------------------------------------------------------------
	void RayTracingVK::resetFrame()
	{
		m_rtxState.frame = -1;
	}
	//--------------------------------------------------------------------------------------------------
	void RayTracingVK::renderScene(const VkCommandBuffer& cmdBuf)
	{
		VkExtent2D render_size = { 1080,694 };
		m_rtxState.size = { render_size.width, render_size.height };
		m_render->setPushContants(m_rtxState);
		m_render->run(cmdBuf, render_size, { m_accelStruct.getDescSet(), m_offscreen.getDescSet(), m_scene.getDescSet(), m_descSet });
	}
	//--------------------------------------------------------------------------------------------------
	void RayTracingVK::drawPost(const VkCommandBuffer& cmdBuf)
	{
		LABEL_SCOPE_VK(cmdBuf);
		auto size = nvmath::vec2f(m_size.width, m_size.height);
		auto area = nvmath::vec2f(m_renderRegion.extent.width, m_renderRegion.extent.height);

		VkViewport viewport{ static_cast<float>(m_renderRegion.offset.x),
							static_cast<float>(m_renderRegion.offset.y),
							static_cast<float>(m_size.width),
							static_cast<float>(m_size.height),
							0.0f,
							1.0f };
		VkRect2D   scissor{ m_renderRegion.offset, {m_renderRegion.extent.width, m_renderRegion.extent.height} };
		//[todo]
		//BGFX_VKAPI(vkCmdSetViewport)(cmdBuf, 0, 1, &viewport);
		//BGFX_VKAPI(vkCmdSetScissor)(cmdBuf, 0, 1, &scissor);

		m_offscreen.m_tonemapper.zoom = 1;// m_descaling ? 1.0f / m_descalingLevel : 1.0f;  //[todo]
		//m_offscreen.m_tonemapper.renderingRatio = size / area;
		m_offscreen.run(cmdBuf);

	}
	//--------------------------------------------------------------------------------------------------
	uint32_t RayTracingVK::getMemoryType(uint32_t typeBits, const VkMemoryPropertyFlags& properties) const
	{
		VkPhysicalDeviceMemoryProperties memoryProperties;
		BGFX_VKAPI(vkGetPhysicalDeviceMemoryProperties)(m_physicalDevice, &memoryProperties);

		for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
		{
			if (((typeBits & (1 << i)) > 0) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				return i;
			}
		}
		std::string err = "Unable to find memory type " + std::to_string(properties);
		LOGE(err.c_str());
		assert(0);
		return ~0u;
	}
	//--------------------------------------------------------------------------------------------------
	void RayTracingVK::addFramebuffer(VkFramebuffer _fbo)
	{
		m_framebuffers.push_back(_fbo);
	}
	//--------------------------------------------------------------------------------------------------
	void RayTracingVK::createCommandPool(uint32_t _queueFamilyIndex) {
		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = _queueFamilyIndex;

		if (BGFX_VKAPI(vkCreateCommandPool)(m_device, &poolInfo, nullptr, &m_cmdPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create command pool!");
		}
	}
	//--------------------------------------------------------------------------------------------------
	void RayTracingVK::createCommandBuffers()
	{
		m_commandBuffers.resize(m_framebuffers.size());
		for (size_t i = 0; i < m_framebuffers.size(); i++)
		{
			VkCommandBufferAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
			allocateInfo.commandPool = m_cmdPool;   //todo
			allocateInfo.commandBufferCount = 1;
			allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			BGFX_VKAPI(vkAllocateCommandBuffers)(m_device, &allocateInfo, &m_commandBuffers[i]);
		}
	}
	//--------------------------------------------------------------------------------------------------
	void RayTracingVK::createSyncObjects(uint32_t _maxframe, uint32_t _imagesize)
	{
		imageAvailableSemaphores.resize(_maxframe);
		renderFinishedSemaphores.resize(_maxframe);
		inFlightFences.resize(_maxframe);
		imagesInFlight.resize(_imagesize, VK_NULL_HANDLE);

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < _maxframe; i++) {
			if (BGFX_VKAPI(vkCreateSemaphore)(m_device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
				BGFX_VKAPI(vkCreateSemaphore)(m_device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
				BGFX_VKAPI(vkCreateFence)(m_device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create synchronization objects for a frame!");
			}
		}
	}
	//--------------------------------------------------------------------------------------------------
	void RayTracingVK::setRenderRegion(const VkRect2D& size)
	{
		if (memcmp(&m_renderRegion, &size, sizeof(VkRect2D)) != 0)
			resetFrame();
		m_renderRegion = size;
	}
	//--------------------------------------------------------------------------------------------------
	void RayTracingVK::submit(VkCommandBuffer cmdbuff,VkQueue graphicsQueue, uint32_t currentFrame, uint32_t imageIndex)
	{
		//uint32_t imageIndex = m_swapChain.getActiveImageIndex();
		//BGFX_VKAPI(vkResetFences)(m_device, 1, &waitFence);

		// In case of using NVLINK
		const uint32_t                deviceMask = 0b0000'0001;// m_useNvlink ? 0b0000'0011 : 0b0000'0001;
		const std::array<uint32_t, 2> deviceIndex = { 0, 1 };

		VkDeviceGroupSubmitInfo deviceGroupSubmitInfo{ VK_STRUCTURE_TYPE_DEVICE_GROUP_SUBMIT_INFO_KHR };
		deviceGroupSubmitInfo.waitSemaphoreCount = 1;
		deviceGroupSubmitInfo.commandBufferCount = 1;
		deviceGroupSubmitInfo.pCommandBufferDeviceMasks = &deviceMask;
		deviceGroupSubmitInfo.signalSemaphoreCount = 1;//m_useNvlink ? 2 : 1;
		deviceGroupSubmitInfo.pSignalSemaphoreDeviceIndices = deviceIndex.data();
		deviceGroupSubmitInfo.pWaitSemaphoreDeviceIndices = deviceIndex.data();

		VkSemaphore semaphoreRead = { imageAvailableSemaphores[currentFrame] };
		VkSemaphore semaphoreWrite = { renderFinishedSemaphores[currentFrame] };

		// Pipeline stage at which the queue submission will wait (via pWaitSemaphores)
		const VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		// The submit info structure specifies a command buffer queue submission batch
		VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
		submitInfo.pWaitDstStageMask = &waitStageMask;  // Pointer to the list of pipeline stages that the semaphore waits will occur at
		submitInfo.pWaitSemaphores = &semaphoreRead;  // Semaphore(s) to wait upon before the submitted command buffer starts executing
		submitInfo.waitSemaphoreCount = 1;                // One wait semaphore
		submitInfo.pSignalSemaphores = &semaphoreWrite;  // Semaphore(s) to be signaled when command buffers have completed
		submitInfo.signalSemaphoreCount = 1;                // One signal semaphore
		submitInfo.pCommandBuffers = &cmdbuff;  // Command buffers(s) to execute in this batch (submission)
		submitInfo.commandBufferCount = 1;                           // One command buffer
		//submitInfo.pNext = &deviceGroupSubmitInfo;

		BGFX_VKAPI(vkResetFences)(m_device, 1, &inFlightFences[currentFrame]);

		// Submit to the graphics queue passing a wait fence
		BGFX_VKAPI(vkQueueSubmit)(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]);

		// Presenting frame
		//m_swapChain.present(m_queue);

	}
	//--------------------------------------------------------------------------------------------------
	void RayTracingVK::createRenderPass(VkFormat swapChainImageFormat)
	{
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = swapChainImageFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		if (BGFX_VKAPI(vkCreateRenderPass)(m_device, &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS) {
			throw std::runtime_error("failed to create render pass!");
		}
	}

}
