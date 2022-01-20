#pragma once
#include <vulkan-local/vulkan.h>
#include <vulkan-local/vulkan_core.h>
#include <vector>
#include <memory>

namespace bgfx {

	class MemHandleBase
	{
	public:
		virtual ~MemHandleBase() = default;  // force the class to become virtual
	};

	typedef MemHandleBase* MemHandle;

	// Objects
	struct Buffer
	{
		VkBuffer  buffer = VK_NULL_HANDLE;
		MemHandle memHandle{ nullptr };
	};

	struct Image
	{
		VkImage   image = VK_NULL_HANDLE;
		MemHandle memHandle{ nullptr };
	};

	struct Texture
	{
		VkImage               image = VK_NULL_HANDLE;
		MemHandle             memHandle{ nullptr };
		VkDescriptorImageInfo descriptor{};
	};

	struct VertexObj
	{
		float pos[3];
		float nrm[3];
		float color[3];
		float texCoord[2];
	};

	// The OBJ model
	struct ObjModel
	{
		uint32_t     nbIndices{ 0 };
		uint32_t     nbVertices{ 0 };
		Buffer vertexBuffer;    // Device buffer of all 'Vertex'
		Buffer indexBuffer;     // Device buffer of the indices forming triangles
		Buffer matColorBuffer;  // Device buffer of array of 'Wavefront material'
		Buffer matIndexBuffer;  // Device buffer of array of 'Wavefront material'
	};

	struct ObjInstance
	{
		float		  transform[16];    // Matrix of the instance
		uint32_t      objIndex{ 0 };  // Model index reference
	};


	struct BlasInput
	{
		// Data used to build acceleration structure geometry
		std::vector<VkAccelerationStructureGeometryKHR>       asGeometry;
		std::vector<VkAccelerationStructureBuildRangeInfoKHR> asBuildOffsetInfo;
		VkBuildAccelerationStructureFlagsKHR                  flags{ 0 };
	};

	struct AccelKHR
	{
		VkAccelerationStructureKHR accel = VK_NULL_HANDLE;
		Buffer               buffer;
	};

	struct BuildAccelerationStructure
	{
		VkAccelerationStructureBuildGeometryInfoKHR buildInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
		VkAccelerationStructureBuildSizesInfoKHR sizeInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
		const VkAccelerationStructureBuildRangeInfoKHR* rangeInfo;
		AccelKHR                                  as;  // result acceleration structure
		AccelKHR                                  cleanupAS;
	};

	class StagingMemoryManager
	{
	public:
		// closes the batch of staging resources since last finalize call
		// and associates it with a fence for later release.
		void finalizeResources(VkFence fence = VK_NULL_HANDLE);

		// releases the staging resources whose fences have completed
		// and those who had no fence at all, skips resourceSets.
		void releaseResources();


	};

	class CommandPool
	{
	public:
		void init(VkDevice                 device,
			uint32_t                 familyIndex,
			VkCommandPoolCreateFlags flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
			VkQueue                  defaultQueue = VK_NULL_HANDLE);

		void deinit();

		VkCommandBuffer createCommandBuffer(VkCommandBufferLevel      level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			bool                      begin = true,
			VkCommandBufferUsageFlags flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
			const VkCommandBufferInheritanceInfo* pInheritanceInfo = nullptr);

		void submit(size_t count, const VkCommandBuffer* cmds, VkQueue queue, VkFence fence = VK_NULL_HANDLE);
		void submit(size_t count, const VkCommandBuffer* cmds, VkFence fence = VK_NULL_HANDLE);
		void submit(const std::vector<VkCommandBuffer>& cmds, VkFence fence = VK_NULL_HANDLE);

		void submitAndWait(size_t count, const VkCommandBuffer* cmds, VkQueue queue);
		void submitAndWait(const std::vector<VkCommandBuffer>& cmds, VkQueue queue)
		{
			submitAndWait(cmds.size(), cmds.data(), queue);
		}
		void submitAndWait(VkCommandBuffer cmd, VkQueue queue) { submitAndWait(1, &cmd, queue); }

		// ends and submits to default queue, waits for queue idle and destroys cmds
		void submitAndWait(size_t count, const VkCommandBuffer* cmds) { submitAndWait(count, cmds, m_queue); }
		void submitAndWait(const std::vector<VkCommandBuffer>& cmds) { submitAndWait(cmds.size(), cmds.data(), m_queue); }
		void submitAndWait(VkCommandBuffer cmd) { submitAndWait(1, &cmd, m_queue); }

	private:
		VkDevice      m_device = VK_NULL_HANDLE;
		VkQueue       m_queue = VK_NULL_HANDLE;
		VkCommandPool m_commandPool = VK_NULL_HANDLE;
	};

	class ResourceAllocator
	{
	public:
		Buffer createBuffer(const VkBufferCreateInfo&   info_,
			const VkMemoryPropertyFlags memUsage_ = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		Buffer createBuffer(VkDeviceSize                size_ = 0,
			VkBufferUsageFlags          usage_ = VkBufferUsageFlags(),
			const VkMemoryPropertyFlags memUsage_ = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		AccelKHR createAcceleration(VkAccelerationStructureCreateInfoKHR& accel_);

		void finalizeAndReleaseStaging(VkFence fence = VK_NULL_HANDLE);

		// Destroy
//
		void destroy(Buffer& b_);
		void destroy(Image& i_);
		void destroy(AccelKHR& a_);
		void destroy(Texture& t_);

	protected:
		std::unique_ptr<StagingMemoryManager> m_staging;
	};



	std::vector<ObjModel>    m_objModel;   // Model on host
	std::vector<ObjInstance> m_instances;  // Scene model instances

	std::vector<AccelKHR> m_blas;

	CommandPool				 m_cmdPool;
	ResourceAllocator*       m_alloc{ nullptr };

	VkDevice                 m_device{ VK_NULL_HANDLE };
	uint32_t                 m_queueIndex{ 0 };


	void createBottomLevelAS();               //done

	void createTopLevelAS();

	void createRtDescriptorSet();

	void createRtShaderBindingTable();
}
