#pragma once
#include <mutex>
#include "vulkan-local/vulkan_core.h"

#if VK_KHR_acceleration_structure

#include "resource_allocator.h"
#include "../nvvk/commands_vk.h"  // this is only needed here to satisfy some samples that rely on it
#include "debug_util_vk.h"
#include "../nvh/nvprint.h"  // this is only needed here to satisfy some samples that rely on it
#include "../nvmath/nvmath.h"
#include <type_traits>

namespace bgfx {

	// Convert a Mat4x4 to the matrix required by acceleration structures
	inline VkTransformMatrixKHR toTransformMatrixKHR(nvmath::mat4f matrix)
	{
		nvmath::mat4f        temp = nvmath::transpose(matrix);
		VkTransformMatrixKHR out_matrix;
		memcpy(&out_matrix, &temp, sizeof(VkTransformMatrixKHR));
		return out_matrix;
	}

	// Ray tracing BLAS and TLAS builder
	class RaytracingBuilderKHR
	{
	public:
		// Inputs used to build Bottom-level acceleration structure.
		// You manage the lifetime of the buffer(s) referenced by the VkAccelerationStructureGeometryKHRs within.
		// In particular, you must make sure they are still valid and not being modified when the BLAS is built or updated.
		struct BlasInput
		{
			// Data used to build acceleration structure geometry
			std::vector<VkAccelerationStructureGeometryKHR>       asGeometry;
			std::vector<VkAccelerationStructureBuildRangeInfoKHR> asBuildOffsetInfo;
			VkBuildAccelerationStructureFlagsKHR                  flags{ 0 };
		};

		// Initializing the allocator and querying the raytracing properties
		void setup(const VkDevice& device, bgfx::ResourceAllocator* allocator, uint32_t queueIndex);

		// Destroying all allocations
		void destroy();

		// Returning the constructed top-level acceleration structure
		VkAccelerationStructureKHR getAccelerationStructure() const;

		// Return the Acceleration Structure Device Address of a BLAS Id
		VkDeviceAddress getBlasDeviceAddress(uint32_t blasId);

		// Create all the BLAS from the vector of BlasInput
		void buildBlas(const std::vector<BlasInput>& input,
			VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);

		// Refit BLAS number blasIdx from updated buffer contents.
		void updateBlas(uint32_t blasIdx, BlasInput& blas, VkBuildAccelerationStructureFlagsKHR flags);

		// Build TLAS for static acceleration structures
		void buildTlas(const std::vector<VkAccelerationStructureInstanceKHR>& instances,
			VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
			bool                                 update = false);

#ifdef VK_NV_ray_tracing_motion_blur
		// Build TLAS for mix of motion and static acceleration structures
		void buildTlas(const std::vector<VkAccelerationStructureMotionInstanceNV>& instances,
			VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_MOTION_BIT_NV,
			bool                                 update = false);
#endif

		// Build TLAS from an array of VkAccelerationStructureInstanceKHR
		// - Use motion=true with VkAccelerationStructureMotionInstanceNV
		// - The resulting TLAS will be stored in m_tlas
		// - update is to rebuild the Tlas with updated matrices, flag must have the 'allow_update'
		template <class T>
		void buildTlas(const std::vector<T>& instances,
			VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
			bool                                 update = false,
			bool                                 motion = false)
		{
			// Cannot call buildTlas twice except to update.
			assert(m_tlas.accel == VK_NULL_HANDLE || update);
			uint32_t countInstance = static_cast<uint32_t>(instances.size());

			// Command buffer to create the TLAS
			bgfx::CommandPool genCmdBuf(m_device, m_queueIndex);
			VkCommandBuffer   cmdBuf = genCmdBuf.createCommandBuffer();

			// Create a buffer holding the actual instance data (matrices++) for use by the AS builder
			bgfx::Buffer instancesBuffer;  // Buffer of instances containing the matrices and BLAS ids
			instancesBuffer = m_alloc->createBuffer(cmdBuf, instances,
				VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
				| VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR);
			NAME_VK(instancesBuffer.buffer);
			VkBufferDeviceAddressInfo bufferInfo{ VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, nullptr, instancesBuffer.buffer };
			VkDeviceAddress           instBufferAddr = BGFX_VKAPI(vkGetBufferDeviceAddress)(m_device, &bufferInfo);

			// Make sure the copy of the instance buffer are copied before triggering the acceleration structure build
			VkMemoryBarrier barrier{ VK_STRUCTURE_TYPE_MEMORY_BARRIER };
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
			BGFX_VKAPI(vkCmdPipelineBarrier)(cmdBuf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
				0, 1, &barrier, 0, nullptr, 0, nullptr);

			// Creating the TLAS
			bgfx::Buffer scratchBuffer;
			cmdCreateTlas(cmdBuf, countInstance, instBufferAddr, scratchBuffer, flags, update, motion);

			// Finalizing and destroying temporary data
			genCmdBuf.submitAndWait(cmdBuf);  // queueWaitIdle inside.
			m_alloc->finalizeAndReleaseStaging();
			m_alloc->destroy(scratchBuffer);
			m_alloc->destroy(instancesBuffer);
		}

		// Creating the TLAS, called by buildTlas
		void cmdCreateTlas(VkCommandBuffer                      cmdBuf,          // Command buffer
			uint32_t                             countInstance,   // number of instances
			VkDeviceAddress                      instBufferAddr,  // Buffer address of instances
			bgfx::Buffer& scratchBuffer,   // Scratch buffer for construction
			VkBuildAccelerationStructureFlagsKHR flags,           // Build creation flag
			bool                                 update,          // Update == animation
			bool                                 motion           // Motion Blur
		);

	protected:
		std::vector<bgfx::AccelKHR> m_blas;  // Bottom-level acceleration structure
		bgfx::AccelKHR              m_tlas;  // Top-level acceleration structure

		// Setup
		VkDevice                 m_device{ VK_NULL_HANDLE };
		uint32_t                 m_queueIndex{ 0 };
		bgfx::ResourceAllocator* m_alloc{ nullptr };
		bgfx::DebugUtil          m_debug;
		bgfx::CommandPool        m_cmdPool;

		struct BuildAccelerationStructure
		{
			VkAccelerationStructureBuildGeometryInfoKHR buildInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
			VkAccelerationStructureBuildSizesInfoKHR sizeInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
			const VkAccelerationStructureBuildRangeInfoKHR* rangeInfo;
			bgfx::AccelKHR                                  as;  // result acceleration structure
			bgfx::AccelKHR                                  cleanupAS;
		};


		void cmdCreateBlas(VkCommandBuffer                          cmdBuf,
			std::vector<uint32_t>                    indices,
			std::vector<BuildAccelerationStructure>& buildAs,
			VkDeviceAddress                          scratchAddress,
			VkQueryPool                              queryPool);
		void cmdCompactBlas(VkCommandBuffer cmdBuf, std::vector<uint32_t> indices, std::vector<BuildAccelerationStructure>& buildAs, VkQueryPool queryPool);
		void destroyNonCompacted(std::vector<uint32_t> indices, std::vector<BuildAccelerationStructure>& buildAs);
		bool hasFlag(VkFlags item, VkFlags flag) { return (item & flag) == flag; }
	};

}  // namespace bgfx


#else
#error This include requires VK_KHR_acceleration_structure support in the Vulkan SDK.
#endif
