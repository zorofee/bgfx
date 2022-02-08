#pragma once
#include <map>
#define BGFX_VKAPI(func) (*(PFN_##func*)FunctionMapVk::Get()->getFunction(EVkFunctionName::func))

namespace bgfx {

	enum EVkFunctionName {
		vkAllocateCommandBuffers,
		vkAllocateDescriptorSets,
		vkAllocateMemory,
		vkBeginCommandBuffer,
		vkBindAccelerationStructureMemoryNV,
		vkBindImageMemory2,
		vkBindBufferMemory,
		vkBindBufferMemory2,
		vkBindImageMemory,
		vkCmdBlitImage,
		vkCmdBuildAccelerationStructuresKHR,
		vkCmdCopyAccelerationStructureKHR,
		vkCmdCopyBuffer,
		vkCmdCopyBufferToImage,
		vkCmdCopyImageToBuffer,
		vkCmdPipelineBarrier,
		vkCmdWriteAccelerationStructuresPropertiesKHR,
		vkCreateAccelerationStructureNV,
		vkCreateAccelerationStructureKHR,
		vkCreateBuffer,
		vkCreateCommandPool,
		vkCreateDescriptorPool,
		vkCreateDescriptorSetLayout,
		vkCreateFence,
		vkCreateImage,
		vkCreateImageView,
		vkCreatePipelineCache,
		vkCreatePipelineLayout,
		vkCreateQueryPool,
		vkCreateRayTracingPipelinesKHR,
		vkCreateSampler,
		vkDestroyAccelerationStructureNV,
		vkDestroyAccelerationStructureKHR,
		vkDestroyBuffer,
		vkDestroyCommandPool,
		vkDestroyDescriptorPool,
		vkDestroyDescriptorSetLayout,
		vkDestroyFence,
		vkDestroyImage,
		vkDestroyImageView,
		vkDestroyPipelineLayout,
		vkDestroyQueryPool,
		vkDestroyShaderModule,
		vkDestroySampler,
		vkEndCommandBuffer,
		vkEnumeratePhysicalDevices,
		vkFreeCommandBuffers,
		vkFreeMemory,
		vkGetAccelerationStructureBuildSizesKHR,
		vkGetAccelerationStructureDeviceAddressKHR,
		vkGetAccelerationStructureMemoryRequirementsNV,
		vkGetBufferDeviceAddress,
		vkGetBufferMemoryRequirements2,
		vkGetDeviceQueue,
		vkGetImageMemoryRequirements2,
		vkGetPhysicalDeviceFeatures,
		vkGetPhysicalDeviceMemoryProperties,
		vkGetPhysicalDeviceProperties2,
		vkGetQueryPoolResults,
		vkGetFenceStatus,
		vkMapMemory,
		vkQueueSubmit,
		vkQueueWaitIdle,
		vkResetFences,
		vkResetCommandPool,
		vkResetQueryPool,
		vkSetDebugUtilsObjectNameEXT,
		vkUnmapMemory,
		vkUpdateDescriptorSets,
		vkWaitForFences,
		vkCreateShaderModule,
		vkDestroyPipeline,
		vkCmdBindPipeline,
		vkCmdBindDescriptorSets,
		vkCmdPushConstants,
		vkCmdTraceRaysKHR,
		vkCreateDeferredOperationKHR,
		vkDestroyDeferredOperationKHR,
		vkGetDeferredOperationMaxConcurrencyKHR,
		vkGetDeferredOperationResultKHR,
		vkDeferredOperationJoinKHR,
		vkGetRayTracingShaderGroupHandlesKHR,
	};

	class FunctionMapVk
	{
	private:
		static FunctionMapVk* m_instance;

	public:
		static FunctionMapVk* Get();

		void setup(std::map<EVkFunctionName, void*>& funcMap);

		void* getFunction(EVkFunctionName funcName);

	public:
		std::map<EVkFunctionName, void*> m_funcMap;
	};

}
