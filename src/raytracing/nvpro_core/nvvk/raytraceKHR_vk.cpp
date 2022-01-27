
#include "raytraceKHR_vk.h"
#include <numeric>

//--------------------------------------------------------------------------------------------------
// Initializing the allocator and querying the raytracing properties
//
void bgfx::RaytracingBuilderKHR::setup(const VkDevice& device, bgfx::ResourceAllocator* allocator, uint32_t queueIndex)
{
	m_device = device;
	m_queueIndex = queueIndex;
	m_debug.setup(device);
	m_alloc = allocator;
}

//--------------------------------------------------------------------------------------------------
// Destroying all allocations
//
void bgfx::RaytracingBuilderKHR::destroy()
{
	if (m_alloc)
	{
		for (auto& b : m_blas)
		{
			m_alloc->destroy(b);
		}
		m_alloc->destroy(m_tlas);
	}
	m_blas.clear();
}

//--------------------------------------------------------------------------------------------------
// Returning the constructed top-level acceleration structure
//
VkAccelerationStructureKHR bgfx::RaytracingBuilderKHR::getAccelerationStructure() const
{
	return m_tlas.accel;
}

//--------------------------------------------------------------------------------------------------
// Return the device address of a Blas previously created.
//
VkDeviceAddress bgfx::RaytracingBuilderKHR::getBlasDeviceAddress(uint32_t blasId)
{
	assert(size_t(blasId) < m_blas.size());
	VkAccelerationStructureDeviceAddressInfoKHR addressInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR };
	addressInfo.accelerationStructure = m_blas[blasId].accel;
	return BGFX_VKAPI(vkGetAccelerationStructureDeviceAddressKHR)(m_device, &addressInfo);
}

//--------------------------------------------------------------------------------------------------
// Create all the BLAS from the vector of BlasInput
// - There will be one BLAS per input-vector entry
// - There will be as many BLAS as input.size()
// - The resulting BLAS (along with the inputs used to build) are stored in m_blas,
//   and can be referenced by index.
// - if flag has the 'Compact' flag, the BLAS will be compacted
//
void bgfx::RaytracingBuilderKHR::buildBlas(const std::vector<BlasInput>& input, VkBuildAccelerationStructureFlagsKHR flags)
{
	m_cmdPool.init(m_device, m_queueIndex);
	auto         nbBlas = static_cast<uint32_t>(input.size());
	VkDeviceSize asTotalSize{ 0 };     // Memory size of all allocated BLAS
	uint32_t     nbCompactions{ 0 };   // Nb of BLAS requesting compaction
	VkDeviceSize maxScratchSize{ 0 };  // Largest scratch size

	// Preparing the information for the acceleration build commands.
	std::vector<BuildAccelerationStructure> buildAs(nbBlas);
	for (uint32_t idx = 0; idx < nbBlas; idx++)
	{
		// Filling partially the VkAccelerationStructureBuildGeometryInfoKHR for querying the build sizes.
		// Other information will be filled in the createBlas (see #2)
		buildAs[idx].buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		buildAs[idx].buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
		buildAs[idx].buildInfo.flags = input[idx].flags | flags;
		buildAs[idx].buildInfo.geometryCount = static_cast<uint32_t>(input[idx].asGeometry.size());
		buildAs[idx].buildInfo.pGeometries = input[idx].asGeometry.data();

		// Build range information
		buildAs[idx].rangeInfo = input[idx].asBuildOffsetInfo.data();

		// Finding sizes to create acceleration structures and scratch
		std::vector<uint32_t> maxPrimCount(input[idx].asBuildOffsetInfo.size());
		for (auto tt = 0; tt < input[idx].asBuildOffsetInfo.size(); tt++)
			maxPrimCount[tt] = input[idx].asBuildOffsetInfo[tt].primitiveCount;  // Number of primitives/triangles
		BGFX_VKAPI(vkGetAccelerationStructureBuildSizesKHR)(m_device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
			&buildAs[idx].buildInfo, maxPrimCount.data(), &buildAs[idx].sizeInfo);

		// Extra info
		asTotalSize += buildAs[idx].sizeInfo.accelerationStructureSize;
		maxScratchSize = std::max(maxScratchSize, buildAs[idx].sizeInfo.buildScratchSize);
		nbCompactions += hasFlag(buildAs[idx].buildInfo.flags, VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR);
	}

	// Allocate the scratch buffers holding the temporary data of the acceleration structure builder
	bgfx::Buffer scratchBuffer =
		m_alloc->createBuffer(maxScratchSize, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
	VkBufferDeviceAddressInfo bufferInfo{ VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, nullptr, scratchBuffer.buffer };
	VkDeviceAddress           scratchAddress = BGFX_VKAPI(vkGetBufferDeviceAddress)(m_device, &bufferInfo);
	NAME_VK(scratchBuffer.buffer);

	// Allocate a query pool for storing the needed size for every BLAS compaction.
	VkQueryPool queryPool{ VK_NULL_HANDLE };
	if (nbCompactions > 0)  // Is compaction requested?
	{
		assert(nbCompactions == nbBlas);  // Don't allow mix of on/off compaction
		VkQueryPoolCreateInfo qpci{ VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };
		qpci.queryCount = nbBlas;
		qpci.queryType = VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR;
		BGFX_VKAPI(vkCreateQueryPool)(m_device, &qpci, nullptr, &queryPool);
	}

	// Batching creation/compaction of BLAS to allow staying in restricted amount of memory
	std::vector<uint32_t> indices;  // Indices of the BLAS to create
	VkDeviceSize          batchSize{ 0 };
	VkDeviceSize          batchLimit{ 256'000'000 };  // 256 MB
	for (uint32_t idx = 0; idx < nbBlas; idx++)
	{
		indices.push_back(idx);
		batchSize += buildAs[idx].sizeInfo.accelerationStructureSize;
		// Over the limit or last BLAS element
		if (batchSize >= batchLimit || idx == nbBlas - 1)
		{
			VkCommandBuffer cmdBuf = m_cmdPool.createCommandBuffer();
			cmdCreateBlas(cmdBuf, indices, buildAs, scratchAddress, queryPool);
			m_cmdPool.submitAndWait(cmdBuf);

			if (queryPool)
			{
				VkCommandBuffer cmdBuf = m_cmdPool.createCommandBuffer();
				cmdCompactBlas(cmdBuf, indices, buildAs, queryPool);
				m_cmdPool.submitAndWait(cmdBuf);  // Submit command buffer and call vkQueueWaitIdle

				// Destroy the non-compacted version
				destroyNonCompacted(indices, buildAs);
			}
			// Reset

			batchSize = 0;
			indices.clear();
		}
	}

	// Logging reduction
	if (queryPool)
	{
		VkDeviceSize compactSize = std::accumulate(buildAs.begin(), buildAs.end(), 0ULL, [](const auto& a, const auto& b) {
			return a + b.sizeInfo.accelerationStructureSize;
			});
		LOGI(" RT BLAS: reducing from: %u to: %u = %u (%2.2f%s smaller) \n", asTotalSize, compactSize,
			asTotalSize - compactSize, (asTotalSize - compactSize) / float(asTotalSize) * 100.f, "%");
	}

	// Keeping all the created acceleration structures
	for (auto& b : buildAs)
	{
		m_blas.emplace_back(b.as);
	}

	// Clean up
	BGFX_VKAPI(vkDestroyQueryPool)(m_device, queryPool, nullptr);
	m_alloc->finalizeAndReleaseStaging();
	m_alloc->destroy(scratchBuffer);
	m_cmdPool.deinit();
}


//--------------------------------------------------------------------------------------------------
// Creating the bottom level acceleration structure for all indices of `buildAs` vector.
// The array of BuildAccelerationStructure was created in buildBlas and the vector of
// indices limits the number of BLAS to create at once. This limits the amount of
// memory needed when compacting the BLAS.
void bgfx::RaytracingBuilderKHR::cmdCreateBlas(VkCommandBuffer                          cmdBuf,
	std::vector<uint32_t>                    indices,
	std::vector<BuildAccelerationStructure>& buildAs,
	VkDeviceAddress                          scratchAddress,
	VkQueryPool                              queryPool)
{
	if (queryPool)  // For querying the compaction size
		BGFX_VKAPI(vkResetQueryPool)(m_device, queryPool, 0, static_cast<uint32_t>(indices.size()));
	uint32_t queryCnt{ 0 };

	for (const auto& idx : indices)
	{
		// Actual allocation of buffer and acceleration structure.
		VkAccelerationStructureCreateInfoKHR createInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
		createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		createInfo.size = buildAs[idx].sizeInfo.accelerationStructureSize;  // Will be used to allocate memory.
		buildAs[idx].as = m_alloc->createAcceleration(createInfo);
		NAME_IDX_VK(buildAs[idx].as.accel, idx);
		NAME_IDX_VK(buildAs[idx].as.buffer.buffer, idx);

		// BuildInfo #2 part
		buildAs[idx].buildInfo.dstAccelerationStructure = buildAs[idx].as.accel;  // Setting where the build lands
		buildAs[idx].buildInfo.scratchData.deviceAddress = scratchAddress;  // All build are using the same scratch buffer

		// Building the bottom-level-acceleration-structure
		BGFX_VKAPI(vkCmdBuildAccelerationStructuresKHR)(cmdBuf, 1, &buildAs[idx].buildInfo, &buildAs[idx].rangeInfo);

		// Since the scratch buffer is reused across builds, we need a barrier to ensure one build
		// is finished before starting the next one.
		VkMemoryBarrier barrier{ VK_STRUCTURE_TYPE_MEMORY_BARRIER };
		barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
		barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
		BGFX_VKAPI(vkCmdPipelineBarrier)(cmdBuf, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
			VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &barrier, 0, nullptr, 0, nullptr);

		if (queryPool)
		{
			// Add a query to find the 'real' amount of memory needed, use for compaction
			BGFX_VKAPI(vkCmdWriteAccelerationStructuresPropertiesKHR)(cmdBuf, 1, &buildAs[idx].buildInfo.dstAccelerationStructure,
				VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR, queryPool, queryCnt++);
		}
	}
}

//--------------------------------------------------------------------------------------------------
// Create and replace a new acceleration structure and buffer based on the size retrieved by the
// Query.
void bgfx::RaytracingBuilderKHR::cmdCompactBlas(VkCommandBuffer                          cmdBuf,
	std::vector<uint32_t>                    indices,
	std::vector<BuildAccelerationStructure>& buildAs,
	VkQueryPool                              queryPool)
{
	uint32_t queryCtn{ 0 };

	// Get the compacted size result back
	std::vector<VkDeviceSize> compactSizes(static_cast<uint32_t>(indices.size()));
	BGFX_VKAPI(vkGetQueryPoolResults)(m_device, queryPool, 0, (uint32_t)compactSizes.size(), compactSizes.size() * sizeof(VkDeviceSize),
		compactSizes.data(), sizeof(VkDeviceSize), VK_QUERY_RESULT_WAIT_BIT);

	for (auto idx : indices)
	{
		buildAs[idx].cleanupAS = buildAs[idx].as;           // previous AS to destroy
		buildAs[idx].sizeInfo.accelerationStructureSize = compactSizes[queryCtn++];  // new reduced size

		// Creating a compact version of the AS
		VkAccelerationStructureCreateInfoKHR asCreateInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
		asCreateInfo.size = buildAs[idx].sizeInfo.accelerationStructureSize;
		asCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		buildAs[idx].as = m_alloc->createAcceleration(asCreateInfo);
		NAME_IDX_VK(buildAs[idx].as.accel, idx);
		NAME_IDX_VK(buildAs[idx].as.buffer.buffer, idx);

		// Copy the original BLAS to a compact version
		VkCopyAccelerationStructureInfoKHR copyInfo{ VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR };
		copyInfo.src = buildAs[idx].buildInfo.dstAccelerationStructure;
		copyInfo.dst = buildAs[idx].as.accel;
		copyInfo.mode = VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_KHR;
		BGFX_VKAPI(vkCmdCopyAccelerationStructureKHR)(cmdBuf, &copyInfo);
	}
}

//--------------------------------------------------------------------------------------------------
// Destroy all the non-compacted acceleration structures
//
void bgfx::RaytracingBuilderKHR::destroyNonCompacted(std::vector<uint32_t> indices, std::vector<BuildAccelerationStructure>& buildAs)
{
	for (auto& i : indices)
	{
		m_alloc->destroy(buildAs[i].cleanupAS);
	}
}
// todo
