/*
 * Copyright (c) 2019-2021, NVIDIA CORPORATION.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-FileCopyrightText: Copyright (c) 2019-2021 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */


#include "debug_util_vk.h"

namespace bgfx {

bool DebugUtil::s_enabled = false;

void DebugUtil::setObjectName(const uint64_t object, const std::string& name, VkObjectType t)
{
	if (s_enabled)
	{
		VkDebugUtilsObjectNameInfoEXT s{ VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT, nullptr, t, object, name.c_str() };
		BGFX_VKAPI(vkSetDebugUtilsObjectNameEXT)(m_device, &s);
	}
}

// clang-format off
void DebugUtil::setObjectName(VkBuffer object, const std::string& name) { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_BUFFER); }
void DebugUtil::setObjectName(VkBufferView object, const std::string& name) { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_BUFFER_VIEW); }
void DebugUtil::setObjectName(VkCommandBuffer object, const std::string& name) { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_COMMAND_BUFFER); }
void DebugUtil::setObjectName(VkCommandPool object, const std::string& name) { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_COMMAND_POOL); }
void DebugUtil::setObjectName(VkDescriptorPool object, const std::string& name) { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_DESCRIPTOR_POOL); }
void DebugUtil::setObjectName(VkDescriptorSet object, const std::string& name) { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_DESCRIPTOR_SET); }
void DebugUtil::setObjectName(VkDescriptorSetLayout object, const std::string& name) { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT); }
void DebugUtil::setObjectName(VkDevice object, const std::string& name) { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_DEVICE); }
void DebugUtil::setObjectName(VkDeviceMemory object, const std::string& name) { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_DEVICE_MEMORY); }
void DebugUtil::setObjectName(VkFramebuffer object, const std::string& name) { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_FRAMEBUFFER); }
void DebugUtil::setObjectName(VkImage object, const std::string& name) { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_IMAGE); }
void DebugUtil::setObjectName(VkImageView object, const std::string& name) { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_IMAGE_VIEW); }
void DebugUtil::setObjectName(VkPipeline object, const std::string& name) { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_PIPELINE); }
void DebugUtil::setObjectName(VkPipelineLayout object, const std::string& name) { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_PIPELINE_LAYOUT); }
void DebugUtil::setObjectName(VkQueryPool object, const std::string& name) { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_QUERY_POOL); }
void DebugUtil::setObjectName(VkQueue object, const std::string& name) { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_QUEUE); }
void DebugUtil::setObjectName(VkRenderPass object, const std::string& name) { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_RENDER_PASS); }
void DebugUtil::setObjectName(VkSampler object, const std::string& name) { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_SAMPLER); }
void DebugUtil::setObjectName(VkSemaphore object, const std::string& name) { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_SEMAPHORE); }
void DebugUtil::setObjectName(VkShaderModule object, const std::string& name) { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_SHADER_MODULE); }
void DebugUtil::setObjectName(VkSwapchainKHR object, const std::string& name) { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_SWAPCHAIN_KHR); }

#if VK_NV_ray_tracing
void DebugUtil::setObjectName(VkAccelerationStructureNV object, const std::string& name) { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV); }
#endif
#if VK_KHR_acceleration_structure
void DebugUtil::setObjectName(VkAccelerationStructureKHR object, const std::string& name) { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR); }
#endif
// clang-format on

}  // namespace bgfx
