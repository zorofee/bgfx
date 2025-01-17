/*
 * Copyright (c) 2018-2021, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2018-2021 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */


#pragma once

#include <vector>
#include <vulkan-local/vulkan_core.h>

namespace bgfx {
/**
  # functions in nvvk

  - findSupportedFormat : returns supported VkFormat from a list of candidates (returns first match)
  - findDepthFormat : returns supported depth format (24, 32, 16-bit)
  - findDepthStencilFormat : returns supported depth-stencil format (24/8, 32/8, 16/8-bit)
  - createRenderPass : wrapper for vkCreateRenderPass

*/
VkFormat findSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
VkFormat findDepthFormat(VkPhysicalDevice physicalDevice);
VkFormat findDepthStencilFormat(VkPhysicalDevice physicalDevice);

//////////////////////////////////////////////////////////////////////////

VkRenderPass createRenderPass(VkDevice                     device,
                              const std::vector<VkFormat>& colorAttachmentFormats,
                              VkFormat                     depthAttachmentFormat,
                              uint32_t                     subpassCount  = 1,
                              bool                         clearColor    = true,
                              bool                         clearDepth    = true,
                              VkImageLayout                initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                              VkImageLayout                finalLayout   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);


#ifdef VULKAN_HPP
inline vk::Format findDepthFormat(vk::PhysicalDevice physicalDevice)
{
  return (vk::Format)findDepthFormat((VkPhysicalDevice)physicalDevice);
}

inline vk::Format findDepthStencilFormat(vk::PhysicalDevice physicalDevice)
{
  return (vk::Format)findDepthStencilFormat((VkPhysicalDevice)physicalDevice);
}

inline vk::Format findSupportedFormat(vk::PhysicalDevice             physicalDevice,
                                      const std::vector<vk::Format>& candidates,
                                      vk::ImageTiling                tiling,
                                      vk::FormatFeatureFlags         features)
{
  return (vk::Format)findSupportedFormat((VkPhysicalDevice)physicalDevice, (const std::vector<VkFormat>&)candidates,
                                         (VkImageTiling)tiling, (VkFormatFeatureFlags)features);
}
inline vk::RenderPass createRenderPass(vk::Device                     device,
                                       const std::vector<vk::Format>& colorAttachmentFormats,
                                       vk::Format                     depthAttachmentFormat,
                                       uint32_t                       subpassCount  = 1,
                                       bool                           clearColor    = true,
                                       bool                           clearDepth    = true,
                                       vk::ImageLayout                initialLayout = vk::ImageLayout::eUndefined,
                                       vk::ImageLayout                finalLayout   = vk::ImageLayout::ePresentSrcKHR)
{
  return (vk::RenderPass)createRenderPass((VkDevice)device, (const std::vector<VkFormat>&)colorAttachmentFormats,
                                          (VkFormat)depthAttachmentFormat, subpassCount, clearColor, clearDepth,
                                          (VkImageLayout)initialLayout, (VkImageLayout)finalLayout);
}
#endif

}  // namespace nvvk
