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

		VkCommandPoolCreateInfo poolCreateInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
		poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		BGFX_VKAPI(vkCreateCommandPool)(m_device, &poolCreateInfo, nullptr, &m_cmdPool);


		m_alloc.init(info.instance, info.device, info.physicalDevice);
		m_debug.setup(m_device);

		m_accelStruct.setup(m_device, info.physicalDevice, m_queues[eCompute].familyIndex, &m_alloc);
		m_offscreen.setup(m_device, info.physicalDevice, m_queues[eTransfer].familyIndex, &m_alloc);
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
		m_render->create(m_size, { m_accelStruct.getDescLayout(), m_offscreen.getDescLayout(), m_scene.getDescLayout(), m_descSetLayout });
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
		attachments[1].format = m_depthFormat;
		attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;

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
	void RayTracingVK::createFrameBuffers()
	{
		// Recreate the frame buffers
		for (auto framebuffer : m_framebuffers)
		{
			BGFX_VKAPI(vkDestroyFramebuffer)(m_device, framebuffer, nullptr);
		}

		// Array of attachment (color, depth)
		std::array<VkImageView, 2> attachments{};

		// Create frame buffers for every swap chain image
		VkFramebufferCreateInfo framebufferCreateInfo{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		framebufferCreateInfo.renderPass = m_renderPass;
		framebufferCreateInfo.attachmentCount = 2;
		framebufferCreateInfo.width = m_size.width;
		framebufferCreateInfo.height = m_size.height;
		framebufferCreateInfo.layers = 1;
		framebufferCreateInfo.pAttachments = attachments.data();

		// Create frame buffers for every swap chain image
		/*
		m_framebuffers.resize(m_swapChain.getImageCount());
		for (uint32_t i = 0; i < m_swapChain.getImageCount(); i++)
		{
			attachments[0] = m_swapChain.getImageView(i);
			attachments[1] = m_depthView;
			BGFX_VKAPI(vkCreateFramebuffer)(m_device, &framebufferCreateInfo, nullptr, &m_framebuffers[i]);
		}
		*/

#ifdef _DEBUG
		for (size_t i = 0; i < m_framebuffers.size(); i++)
		{
			std::string                   name = std::string("AppBase") + std::to_string(i);
			VkDebugUtilsObjectNameInfoEXT nameInfo{ VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
			nameInfo.objectHandle = (uint64_t)m_framebuffers[i];
			nameInfo.objectType = VK_OBJECT_TYPE_FRAMEBUFFER;
			nameInfo.pObjectName = name.c_str();
			BGFX_VKAPI(vkSetDebugUtilsObjectNameEXT)(m_device, &nameInfo);
		}
#endif  // _DEBUG
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
		allocateInfo.commandPool = m_cmdPool;
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
		BGFX_VKAPI(vkFreeCommandBuffers)(m_device, m_cmdPool, 1, &cmdBuffer);

		// Setting up the view
		VkImageViewCreateInfo depthStencilView{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
		depthStencilView.format = m_depthFormat;
		depthStencilView.subresourceRange = subresourceRange;
		depthStencilView.image = m_depthImage;
		BGFX_VKAPI(vkCreateImageView)(m_device, &depthStencilView, nullptr, &m_depthView);
	}
	//--------------------------------------------------------------------------------------------------
	void RayTracingVK::createCommandBuffers()
	{
		int count = 2;  //swapchain.getimagecount()
		VkCommandBufferAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		allocateInfo.commandPool = m_cmdPool;
		allocateInfo.commandBufferCount = count;
		allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		m_commandBuffers.resize(count);
		BGFX_VKAPI(vkAllocateCommandBuffers)(m_device, &allocateInfo, m_commandBuffers.data());
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
		//VkDescriptorBufferInfo            accelImpSmpl{ m_skydome.m_accelImpSmpl.buffer, 0, VK_WHOLE_SIZE };
		writes.emplace_back(m_bind.makeWrite(m_descSet, EnvBindings::eSunSky, &sunskyDesc));
		//writes.emplace_back(m_bind.makeWrite(m_descSet, EnvBindings::eHdr, &m_skydome.m_texHdr.descriptor));
		//writes.emplace_back(m_bind.makeWrite(m_descSet, EnvBindings::eImpSamples, &accelImpSmpl));

		BGFX_VKAPI(vkUpdateDescriptorSets)(m_device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
	}

	//--------------------------------------------------------------------------------------------------
	void RayTracingVK::render(VkFramebuffer fbo,  uint32_t curFrame)
	{
		const VkCommandBuffer& cmdBuf = m_commandBuffers[0];

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
			postRenderPassBeginInfo.framebuffer = fbo;
			postRenderPassBeginInfo.renderArea = { {}, {400,300} };

			BGFX_VKAPI(vkCmdBeginRenderPass)(cmdBuf, &postRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);  //如果加上depth attachment就会报错

			// Draw the rendering result + tonemapper
			drawPost(cmdBuf);

			// Render the UI
			BGFX_VKAPI(vkCmdEndRenderPass)(cmdBuf);
		}

		// Submit for display
		BGFX_VKAPI(vkEndCommandBuffer)(cmdBuf);
		submitFrame();
	}

	//--------------------------------------------------------------------------------------------------
	void RayTracingVK::updateUniformBuffer(const VkCommandBuffer& cmdBuf)
	{
		m_renderRegion = { 1080,694 };
		const float aspectRatio = m_renderRegion.extent.width / static_cast<float>(m_renderRegion.extent.height);

		m_scene.updateCamera(cmdBuf, aspectRatio);
	}
	//--------------------------------------------------------------------------------------------------
	void RayTracingVK::renderScene(const VkCommandBuffer& cmdBuf)
	{
		VkExtent2D render_size = { 1280,720 };
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
		BGFX_VKAPI(vkCmdSetViewport)(cmdBuf, 0, 1, &viewport);
		BGFX_VKAPI(vkCmdSetScissor)(cmdBuf, 0, 1, &scissor);

		m_offscreen.m_tonemapper.zoom = 1;// m_descaling ? 1.0f / m_descalingLevel : 1.0f;  //[todo]
		m_offscreen.m_tonemapper.renderingRatio = size / area;
		m_offscreen.run(cmdBuf);

	}
	//--------------------------------------------------------------------------------------------------
	void RayTracingVK::submitFrame()
	{
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

}
