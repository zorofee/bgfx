#pragma once
#include "vulkan-local/vulkan_core.h"


namespace bgfx {
// Holding the queue created at Vulkan context creation, using nvvk::Context
	struct Queue
	{
	  VkQueue  queue{VK_NULL_HANDLE};
	  uint32_t familyIndex{~0u};
	  uint32_t queueIndex{~0u};
	};

}  // namespace bgfx
