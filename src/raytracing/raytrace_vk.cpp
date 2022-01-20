
#include "raytrace_vk.h"
#include <numeric>
#include <algorithm>
#include <assert.h>

namespace bgfx {

	
	VkDeviceAddress getBufferDeviceAddress(VkDevice device, VkBuffer buffer)
	{
		VkBufferDeviceAddressInfo info = { VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
		info.buffer = buffer;
		return vkGetBufferDeviceAddress(device, &info);
	}


	auto objectToVkGeometryKHR(const ObjModel& model)
	{
		VkDeviceAddress vertexAddress = getBufferDeviceAddress(NULL, NULL);
		VkDeviceAddress indexAddress = getBufferDeviceAddress(NULL,NULL);

		uint32_t maxPrimitiveCount = model.nbIndices / 3;


		// Describe buffer as array of VertexObj.
		VkAccelerationStructureGeometryTrianglesDataKHR triangles{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR };
		triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;  // vec3 vertex position data.
		triangles.vertexData.deviceAddress = vertexAddress;
		triangles.vertexStride = sizeof(VertexObj);
		// Describe index data (32-bit unsigned int)
		triangles.indexType = VK_INDEX_TYPE_UINT32;
		triangles.indexData.deviceAddress = indexAddress;
		// Indicate identity transform by setting transformData to null device pointer.
		//triangles.transformData = {};
		triangles.maxVertex = model.nbVertices;

		// Identify the above data as containing opaque triangles.
		VkAccelerationStructureGeometryKHR asGeom{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
		asGeom.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
		asGeom.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
		asGeom.geometry.triangles = triangles;

		// The entire array will be used to build the BLAS.
		VkAccelerationStructureBuildRangeInfoKHR offset;
		offset.firstVertex = 0;
		offset.primitiveCount = maxPrimitiveCount;
		offset.primitiveOffset = 0;
		offset.transformOffset = 0;

		// Our blas is made from only one geometry, but could be made of many geometries
		BlasInput input;
		input.asGeometry.emplace_back(asGeom);
		input.asBuildOffsetInfo.emplace_back(offset);

		return input;

	}

	bool hasFlag(VkFlags item, VkFlags flag) { return (item & flag) == flag; }

	bool checkResult(VkResult result, const char* file, int32_t line)
	{
		if (result == VK_SUCCESS)
		{
			return false;
		}

		if (result < 0)
		{
			//LOGE("%s(%d): Vulkan Error : %s\n", file, line, getResultString(result));
			assert(!"Critical Vulkan Error");

			return true;
		}

		return false;
	}

	// Convert a Mat4x4 to the matrix required by acceleration structures
	VkTransformMatrixKHR toTransformMatrixKHR(float matrix[16])
	{
		//nvmath::mat4f        temp = nvmath::transpose(matrix);
		VkTransformMatrixKHR out_matrix;
		//memcpy(&out_matrix, &temp, sizeof(VkTransformMatrixKHR));
		return out_matrix;
	}

	void CommandPool::init(VkDevice device, uint32_t familyIndex, VkCommandPoolCreateFlags flags, VkQueue defaultQueue)
	{
		
	}

	VkCommandBuffer CommandPool::createCommandBuffer(VkCommandBufferLevel level /*= VK_COMMAND_BUFFER_LEVEL_PRIMARY*/,
		bool                 begin,
		VkCommandBufferUsageFlags flags /*= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT*/,
		const VkCommandBufferInheritanceInfo* pInheritanceInfo /*= nullptr*/)
	{
		VkCommandBufferAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		allocInfo.level = level;
		allocInfo.commandPool = m_commandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer cmd;
		vkAllocateCommandBuffers(m_device, &allocInfo, &cmd);

		if (begin)
		{
			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = flags;
			beginInfo.pInheritanceInfo = pInheritanceInfo;

			vkBeginCommandBuffer(cmd, &beginInfo);
		}

		return cmd;
	}

	void CommandPool::submitAndWait(size_t count, const VkCommandBuffer* cmds, VkQueue queue)
	{
		submit(count, cmds, queue);
		VkResult result = vkQueueWaitIdle(queue);
		if (checkResult(result, __FILE__, __LINE__))
		{
			exit(-1);
		}
		vkFreeCommandBuffers(m_device, m_commandPool, (uint32_t)count, cmds);
	}

	void CommandPool::submit(size_t count, const VkCommandBuffer* cmds, VkQueue queue, VkFence fence)
	{
		for (size_t i = 0; i < count; i++)
		{
			vkEndCommandBuffer(cmds[i]);
		}

		VkSubmitInfo submit = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		submit.pCommandBuffers = cmds;
		submit.commandBufferCount = (uint32_t)count;
		vkQueueSubmit(queue, 1, &submit, fence);
	}

	void CommandPool::submit(size_t count, const VkCommandBuffer* cmds, VkFence fence)
	{
		submit(count, cmds, m_queue, fence);
	}

	void CommandPool::submit(const std::vector<VkCommandBuffer>& cmds, VkFence fence)
	{
		submit(cmds.size(), cmds.data(), m_queue, fence);
	}

	void CommandPool::deinit()
	{
		if (m_commandPool)
		{
			vkDestroyCommandPool(m_device, m_commandPool, nullptr);
			m_commandPool = VK_NULL_HANDLE;
		}
		m_device = VK_NULL_HANDLE;
	}


	void StagingMemoryManager::finalizeResources(VkFence fence)
	{
	
	}

	void StagingMemoryManager::releaseResources()
	{
		
	}


	Buffer ResourceAllocator::createBuffer(const VkBufferCreateInfo&   info_, const VkMemoryPropertyFlags memUsage_ )
	{
		return Buffer();
	}

	Buffer ResourceAllocator::createBuffer(VkDeviceSize size_, VkBufferUsageFlags usage_, const VkMemoryPropertyFlags memUsage_)
	{
		VkBufferCreateInfo info{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		info.size = size_;
		info.usage = usage_ | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		return createBuffer(info, memUsage_);
	}


	AccelKHR ResourceAllocator::createAcceleration(VkAccelerationStructureCreateInfoKHR& accel_)
	{
		AccelKHR resultAccel;
		// Allocating the buffer to hold the acceleration structure
		resultAccel.buffer = createBuffer(accel_.size, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR
			| VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
		// Setting the buffer
		accel_.buffer = resultAccel.buffer.buffer;
		// Create the acceleration structure
		vkCreateAccelerationStructureKHR(m_device, &accel_, nullptr, &resultAccel.accel);

		return resultAccel;
	}

	void ResourceAllocator::destroy(AccelKHR& a_)
	{
		vkDestroyAccelerationStructureKHR(m_device, a_.accel, nullptr);
		destroy(a_.buffer);

		a_ = AccelKHR();
	}

	void ResourceAllocator::finalizeAndReleaseStaging(VkFence fence /*= VK_NULL_HANDLE*/)
	{
		m_staging->finalizeResources(fence);
		m_staging->releaseResources();
	}

	void cmdCreateBlas(VkCommandBuffer                          cmdBuf,
		std::vector<uint32_t>                    indices,
		std::vector<BuildAccelerationStructure>& buildAs,
		VkDeviceAddress                          scratchAddress,
		VkQueryPool                              queryPool)
	{
		if (queryPool)  // For querying the compaction size
			vkResetQueryPool(m_device, queryPool, 0, static_cast<uint32_t>(indices.size()));
		uint32_t queryCnt{ 0 };

		for (const auto& idx : indices)
		{
			// Actual allocation of buffer and acceleration structure.
			VkAccelerationStructureCreateInfoKHR createInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
			createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
			createInfo.size = buildAs[idx].sizeInfo.accelerationStructureSize;  // Will be used to allocate memory.
			buildAs[idx].as = m_alloc->createAcceleration(createInfo);
			//NAME_IDX_VK(buildAs[idx].as.accel, idx);
			//NAME_IDX_VK(buildAs[idx].as.buffer.buffer, idx);

			// BuildInfo #2 part
			buildAs[idx].buildInfo.dstAccelerationStructure = buildAs[idx].as.accel;  // Setting where the build lands
			buildAs[idx].buildInfo.scratchData.deviceAddress = scratchAddress;  // All build are using the same scratch buffer

			// Building the bottom-level-acceleration-structure
			vkCmdBuildAccelerationStructuresKHR(cmdBuf, 1, &buildAs[idx].buildInfo, &buildAs[idx].rangeInfo);

			// Since the scratch buffer is reused across builds, we need a barrier to ensure one build
			// is finished before starting the next one.
			VkMemoryBarrier barrier{ VK_STRUCTURE_TYPE_MEMORY_BARRIER };
			barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
			barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
			vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
				VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &barrier, 0, nullptr, 0, nullptr);

			if (queryPool)
			{
				// Add a query to find the 'real' amount of memory needed, use for compaction
				vkCmdWriteAccelerationStructuresPropertiesKHR(cmdBuf, 1, &buildAs[idx].buildInfo.dstAccelerationStructure,
					VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR, queryPool, queryCnt++);
			}
		}
	}


	//--------------------------------------------------------------------------------------------------
	// Create and replace a new acceleration structure and buffer based on the size retrieved by the
	// Query.
	void cmdCompactBlas(VkCommandBuffer                          cmdBuf,
		std::vector<uint32_t>                    indices,
		std::vector<BuildAccelerationStructure>& buildAs,
		VkQueryPool                              queryPool)
	{
		uint32_t queryCtn{ 0 };

		// Get the compacted size result back
		std::vector<VkDeviceSize> compactSizes(static_cast<uint32_t>(indices.size()));
		vkGetQueryPoolResults(m_device, queryPool, 0, (uint32_t)compactSizes.size(), compactSizes.size() * sizeof(VkDeviceSize),
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
			//NAME_IDX_VK(buildAs[idx].as.accel, idx);
			//NAME_IDX_VK(buildAs[idx].as.buffer.buffer, idx);

			// Copy the original BLAS to a compact version
			VkCopyAccelerationStructureInfoKHR copyInfo{ VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR };
			copyInfo.src = buildAs[idx].buildInfo.dstAccelerationStructure;
			copyInfo.dst = buildAs[idx].as.accel;
			copyInfo.mode = VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_KHR;
			vkCmdCopyAccelerationStructureKHR(cmdBuf, &copyInfo);
		}
	}

	void destroyNonCompacted(std::vector<uint32_t> indices, std::vector<BuildAccelerationStructure>& buildAs)
	{
		for (auto& i : indices)
		{
			m_alloc->destroy(buildAs[i].cleanupAS);
		}
	}

	void buildBlas(const std::vector<BlasInput>& input, VkBuildAccelerationStructureFlagsKHR flags)
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
			vkGetAccelerationStructureBuildSizesKHR(m_device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
				&buildAs[idx].buildInfo, maxPrimCount.data(), &buildAs[idx].sizeInfo);

			// Extra info
			asTotalSize += buildAs[idx].sizeInfo.accelerationStructureSize;
			maxScratchSize = std::max(maxScratchSize, buildAs[idx].sizeInfo.buildScratchSize);
			nbCompactions += hasFlag(buildAs[idx].buildInfo.flags, VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR);
		}

		// Allocate the scratch buffers holding the temporary data of the acceleration structure builder
		Buffer scratchBuffer = m_alloc->createBuffer(maxScratchSize, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
		VkBufferDeviceAddressInfo bufferInfo{ VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, nullptr, scratchBuffer.buffer };
		VkDeviceAddress           scratchAddress = vkGetBufferDeviceAddress(m_device, &bufferInfo);
		//NAME_VK(scratchBuffer.buffer);

		// Allocate a query pool for storing the needed size for every BLAS compaction.
		VkQueryPool queryPool{ VK_NULL_HANDLE };
		if (nbCompactions > 0)  // Is compaction requested?
		{
			assert(nbCompactions == nbBlas);  // Don't allow mix of on/off compaction
			VkQueryPoolCreateInfo qpci{ VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };
			qpci.queryCount = nbBlas;
			qpci.queryType = VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR;
			vkCreateQueryPool(m_device, &qpci, nullptr, &queryPool);
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
		//	LOGI(" RT BLAS: reducing from: %u to: %u = %u (%2.2f%s smaller) \n", asTotalSize, compactSize,asTotalSize - compactSize, (asTotalSize - compactSize) / float(asTotalSize) * 100.f, "%");
		}

		// Keeping all the created acceleration structures
		for (auto& b : buildAs)
		{
			m_blas.emplace_back(b.as);
		}

		// Clean up
		vkDestroyQueryPool(m_device, queryPool, nullptr);
		m_alloc->finalizeAndReleaseStaging();
		m_alloc->destroy(scratchBuffer);
		m_cmdPool.deinit();
	}

	void buildTlas(const std::vector<VkAccelerationStructureInstanceKHR>& instances, VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,bool update = false)
	{
		
	}

	void createBottomLevelAS()
	{
		std::vector<BlasInput> allBlas;
		allBlas.reserve(m_objModel.size());

		for (const auto& obj : m_objModel)
		{
			auto blas = objectToVkGeometryKHR(obj);

			// We could add more geometry in each BLAS, but we add only one for now
			allBlas.emplace_back(blas);
		}

		buildBlas(allBlas, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
	}


	VkDeviceAddress getBlasDeviceAddress(uint32_t blasId)
	{
		assert(size_t(blasId) < m_blas.size());
		VkAccelerationStructureDeviceAddressInfoKHR addressInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR };
		addressInfo.accelerationStructure = m_blas[blasId].accel;
		return vkGetAccelerationStructureDeviceAddressKHR(m_device, &addressInfo);
	}


	void createTopLevelAS()
	{
		std::vector<VkAccelerationStructureInstanceKHR> tlas;
		tlas.reserve(m_instances.size());
		for (const ObjInstance& inst : m_instances)
		{
			VkAccelerationStructureInstanceKHR rayInst{};
			float test[16];
			rayInst.transform = toTransformMatrixKHR(/*inst.transform*/test);  // Position of the instance
			rayInst.instanceCustomIndex = inst.objIndex;                               // gl_InstanceCustomIndexEXT
			rayInst.accelerationStructureReference = getBlasDeviceAddress(inst.objIndex);
			rayInst.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
			rayInst.mask = 0xFF;       //  Only be hit if rayMask & instance.mask != 0
			rayInst.instanceShaderBindingTableRecordOffset = 0;  // We will use the same hit group for all objects
			tlas.emplace_back(rayInst);
		}
		buildTlas(tlas, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
	}

	void createRtDescriptorSet()
	{

	}

	void createRtShaderBindingTable()
	{

	}

}
