#pragma once
#include <vector>
#include <vulkan-local/vulkan.h>

#if VK_KHR_acceleration_structure
#include "resource_allocator.h"
#include "commands_vk.h"
#include "nvmath.h"

namespace bgfx {

	// Convert a Mat4x4 to the matrix required by acceleration structures
	inline VkTransformMatrixKHR toTransformMatrixKHR(nvmath::mat4f matrix)
	{
		nvmath::mat4f        temp = nvmath::transpose(matrix);
		VkTransformMatrixKHR out_matrix;
		memcpy(&out_matrix, &temp, sizeof(VkTransformMatrixKHR));
		return out_matrix;
	}

	class RayTracingBuilder {
	public:
		struct BlasInput
		{
			// Data used to build acceleration structure geometry
			std::vector<VkAccelerationStructureGeometryKHR>       asGeometry;
			std::vector<VkAccelerationStructureBuildRangeInfoKHR> asBuildOffsetInfo;
			VkBuildAccelerationStructureFlagsKHR                  flags{ 0 };
		};

		// Initializing the allocator and querying the raytracing properties
		void setup(const VkDevice& device, ResourceAllocator* allocator, uint32_t queueIndex);

		// Destroying all allocations
		void destroy();

		// Returning the constructed top-level acceleration structure
		VkAccelerationStructureKHR getAccelerationStructure() const;

		// Return the Acceleration Structure Device Address of a BLAS Id
		VkDeviceAddress getBlasDeviceAddress(uint32_t blasId);

		// Create all the BLAS from the vector of BlasInput
		void buildBlas(const std::vector<BlasInput>&        input,
			VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);

		// Refit BLAS number blasIdx from updated buffer contents.
		void updateBlas(uint32_t blasIdx, BlasInput& blas, VkBuildAccelerationStructureFlagsKHR flags);




	protected:
		struct BuildAccelerationStructure
		{
			VkAccelerationStructureBuildGeometryInfoKHR buildInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
			VkAccelerationStructureBuildSizesInfoKHR sizeInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
			const VkAccelerationStructureBuildRangeInfoKHR* rangeInfo;
			AccelKHR                                  as;  // result acceleration structure
			AccelKHR                                  cleanupAS;
		};

		// Setup
		VkDevice                 m_device{ VK_NULL_HANDLE };
		uint32_t                 m_queueIndex{ 0 };
		ResourceAllocator*		 m_alloc{ nullptr };
		CommandPool*			 m_cmdPool;

	};
}

#endif
