/*
 * Copyright (c) 2021, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2021 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */


#pragma once

#include "nvpro_core/nvvk/resource_allocator.h"
#include "nvpro_core/nvvk/debug_util_vk.h"
#include "nvpro_core/nvvk/descriptorsets_vk.h"
#include "nvpro_core/nvvk/sbtwrapper_vk.h"
#include "nvpro_core/nvvk/profiler_vk.h"

#include "renderer.h"
#include "shaders/host_device.h"

using bgfx::SBTWrapper;
using namespace bgfx;

/*

Creating the RtCore renderer 
* Requiring:  
  - Acceleration structure (AccelSctruct / Tlas)
  - An image (Post StoreImage)
  - The glTF scene (vertex, index, materials, ... )

* Usage
  - setup as usual
  - create
  - run
*/
class RtxPipeline : public Renderer
{
public:
  void setup(const VkDevice& device, const VkPhysicalDevice& physicalDevice, uint32_t familyIndex, bgfx::ResourceAllocator* allocator) override;
  void destroy() override;
  void create(const VkExtent2D& size, const std::vector<VkDescriptorSetLayout>& rtDescSetLayouts) override;
  void run(const VkCommandBuffer& cmdBuf, const VkExtent2D& size, bgfx::ProfilerVK& profiler, const std::vector<VkDescriptorSet>& descSets) override;
  void useAnyHit(bool enable);

  const std::string name() override { return std::string("Rtx"); }

private:
  void createPipeline();
  void createPipelineLayout(const std::vector<VkDescriptorSetLayout>& rtDescSetLayouts);


  uint32_t m_nbHit{1};
  bool     m_enableAnyhit{true};

private:
  // Setup
  bgfx::ResourceAllocator* m_pAlloc;  // Allocator for buffer, images, acceleration structures
  bgfx::DebugUtil          m_debug;   // Utility to name objects
  VkDevice                 m_device;
  uint32_t                 m_queueIndex{0};


  VkPhysicalDeviceRayTracingPipelinePropertiesKHR m_rtProperties{};
  VkPipelineLayout                                m_rtPipelineLayout{VK_NULL_HANDLE};
  VkPipeline                                      m_rtPipeline{VK_NULL_HANDLE};
  SBTWrapper                                      m_stbWrapper;
};
