#pragma once
#include "vulkan-local/vulkan_core.h"

#include <memory>
#include <vector>

#include "memallocator_vk.h"
#include "samplers_vk.h"
#include "stagingmemorymanager_vk.h"
#include "../../functions_vk.h"


namespace bgfx {

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

	struct AccelNV
	{
		VkAccelerationStructureNV accel = VK_NULL_HANDLE;
		MemHandle                 memHandle{ nullptr };
	};

	struct AccelKHR
	{
		VkAccelerationStructureKHR accel = VK_NULL_HANDLE;
		bgfx::Buffer               buffer;
	};

	//--------------------------------------------------------------------------------------------------
	// Allocator for buffers, images and acceleration structures
	//
	class StagingMemoryManager;



	class ResourceAllocator
	{
	public:
		ResourceAllocator(ResourceAllocator const&) = delete;
		ResourceAllocator& operator=(ResourceAllocator const&) = delete;

		ResourceAllocator() = default;
		ResourceAllocator(VkDevice         device,
			VkPhysicalDevice physicalDevice,
			MemAllocator* memAllocator,
			VkDeviceSize     stagingBlockSize = NVVK_DEFAULT_STAGING_BLOCKSIZE);

		// All staging buffers must be cleared before
		virtual ~ResourceAllocator();

		//--------------------------------------------------------------------------------------------------
		// Initialization of the allocator
		void init(VkDevice device, VkPhysicalDevice physicalDevice, MemAllocator* memAlloc, VkDeviceSize stagingBlockSize = NVVK_DEFAULT_STAGING_BLOCKSIZE);

		void deinit();

		MemAllocator* getMemoryAllocator() { return m_memAlloc; }

		//--------------------------------------------------------------------------------------------------
		// Basic buffer creation
		virtual bgfx::Buffer createBuffer(const VkBufferCreateInfo& info_,
			const VkMemoryPropertyFlags memUsage_ = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		//--------------------------------------------------------------------------------------------------
		// Simple buffer creation
		// implicitly sets VK_BUFFER_USAGE_TRANSFER_DST_BIT
		bgfx::Buffer createBuffer(VkDeviceSize                size_ = 0,
			VkBufferUsageFlags          usage_ = VkBufferUsageFlags(),
			const VkMemoryPropertyFlags memUsage_ = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		//--------------------------------------------------------------------------------------------------
		// Simple buffer creation with data uploaded through staging manager
		// implicitly sets VK_BUFFER_USAGE_TRANSFER_DST_BIT
		bgfx::Buffer createBuffer(const VkCommandBuffer& cmdBuf,
			const VkDeviceSize& size_,
			const void* data_,
			VkBufferUsageFlags     usage_,
			VkMemoryPropertyFlags  memProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		//--------------------------------------------------------------------------------------------------
		// Simple buffer creation with data uploaded through staging manager
		// implicitly sets VK_BUFFER_USAGE_TRANSFER_DST_BIT
		template <typename T>
		bgfx::Buffer createBuffer(const VkCommandBuffer& cmdBuf,
			const std::vector<T>& data_,
			VkBufferUsageFlags     usage_,
			VkMemoryPropertyFlags  memProps_ = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
		{
			return createBuffer(cmdBuf, sizeof(T) * data_.size(), data_.data(), usage_, memProps_);
		}


		//--------------------------------------------------------------------------------------------------
		// Basic image creation
		bgfx::Image createImage(const VkImageCreateInfo& info_, const VkMemoryPropertyFlags memUsage_ = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);


		//--------------------------------------------------------------------------------------------------
		// Create an image with data uploaded through staging manager
		bgfx::Image createImage(const VkCommandBuffer& cmdBuf,
			size_t                   size_,
			const void* data_,
			const VkImageCreateInfo& info_,
			const VkImageLayout& layout_ = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		//--------------------------------------------------------------------------------------------------
		// other variants could exist with a few defaults but we already have bgfx::makeImage2DViewCreateInfo()
		// we could always override viewCreateInfo.image
		bgfx::Texture createTexture(const Image& image, const VkImageViewCreateInfo& imageViewCreateInfo);
		bgfx::Texture createTexture(const Image& image, const VkImageViewCreateInfo& imageViewCreateInfo, const VkSamplerCreateInfo& samplerCreateInfo);

		//--------------------------------------------------------------------------------------------------
		// shortcut that creates the image for the texture
		// - creates the image
		// - creates the texture part by associating image and sampler
		//
		bgfx::Texture createTexture(const VkCommandBuffer& cmdBuf,
			size_t                     size_,
			const void* data_,
			const VkImageCreateInfo& info_,
			const VkSamplerCreateInfo& samplerCreateInfo,
			const VkImageLayout& layout_ = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			bool                       isCube = false);

		//--------------------------------------------------------------------------------------------------
		// Create the acceleration structure
		//
		bgfx::AccelNV createAcceleration(VkAccelerationStructureCreateInfoNV& accel_);


		//--------------------------------------------------------------------------------------------------
		// Create the acceleration structure
		//
		bgfx::AccelKHR createAcceleration(VkAccelerationStructureCreateInfoKHR& accel_);

		//--------------------------------------------------------------------------------------------------
		// Acquire a sampler with the provided information (see bgfx::SamplerPool for details).
		// Every acquire must have an appropriate release for appropriate internal reference counting
		VkSampler acquireSampler(const VkSamplerCreateInfo& info);
		void      releaseSampler(VkSampler sampler);

		//--------------------------------------------------------------------------------------------------
		// implicit staging operations triggered by create are managed here
		void finalizeStaging(VkFence fence = VK_NULL_HANDLE);
		void finalizeAndReleaseStaging(VkFence fence = VK_NULL_HANDLE);
		void releaseStaging();

		StagingMemoryManager* getStaging();
		const StagingMemoryManager* getStaging() const;


		//--------------------------------------------------------------------------------------------------
		// Destroy
		//
		void destroy(bgfx::Buffer& b_);
		void destroy(bgfx::Image& i_);
		void destroy(bgfx::AccelNV& a_);
		void destroy(bgfx::AccelKHR& a_);
		void destroy(bgfx::Texture& t_);

		//--------------------------------------------------------------------------------------------------
		// Other
		//
		void* map(const bgfx::Buffer& buffer);
		void  unmap(const bgfx::Buffer& buffer);
		void* map(const bgfx::Image& image);
		void  unmap(const bgfx::Image& image);

		VkDevice         getDevice() const { return m_device; }
		VkPhysicalDevice getPhysicalDevice() const { return m_physicalDevice; }


	protected:
		// If necessary, these can be overriden to specialize the allocation, for instance to
		// enforce allocation of exportable
		virtual MemHandle AllocateMemory(const MemAllocateInfo& allocateInfo);
		virtual void      CreateBufferEx(const VkBufferCreateInfo& info_, VkBuffer* buffer);
		virtual void      CreateImageEx(const VkImageCreateInfo& info_, VkImage* image);

		//--------------------------------------------------------------------------------------------------
		// Finding the memory type for memory allocation
		//
		uint32_t getMemoryType(uint32_t typeBits, const VkMemoryPropertyFlags& properties);

		VkDevice                              m_device{ VK_NULL_HANDLE };
		VkPhysicalDevice                      m_physicalDevice{ VK_NULL_HANDLE };
		VkPhysicalDeviceMemoryProperties      m_memoryProperties{};
		MemAllocator* m_memAlloc{ nullptr };
		std::unique_ptr<StagingMemoryManager> m_staging;
		SamplerPool                           m_samplerPool;


	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class DeviceMemoryAllocator;

	/**
	 \class nvvk::ResourceAllocatorDma
	 nvvk::ResourceAllocatorDMA is a convencience class owning a nvvk::DMAMemoryAllocator and nvvk::DeviceMemoryAllocator object
	*/
	class ResourceAllocatorDma : public ResourceAllocator
	{
	public:
		ResourceAllocatorDma() = default;
		ResourceAllocatorDma(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize stagingBlockSize = NVVK_DEFAULT_STAGING_BLOCKSIZE, VkDeviceSize memBlockSize = 0);
		virtual ~ResourceAllocatorDma();

		void init(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize stagingBlockSize = NVVK_DEFAULT_STAGING_BLOCKSIZE, VkDeviceSize memBlockSize = 0);
		// Provided such that ResourceAllocatorDedicated, ResourceAllocatorDma and ResourceAllocatorVma all have the same interface
		void init(VkInstance, VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize stagingBlockSize = NVVK_DEFAULT_STAGING_BLOCKSIZE, VkDeviceSize memBlockSize = 0);

		void deinit();

		bgfx::DeviceMemoryAllocator* getDMA() { return m_dma.get(); }
		const bgfx::DeviceMemoryAllocator* getDMA() const { return m_dma.get(); }

	protected:
		std::unique_ptr<bgfx::DeviceMemoryAllocator> m_dma;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	/**
	 \class nvvk::ResourceAllocatorDedicated
	 \brief nvvk::ResourceAllocatorDedicated is a convencience class automatically creating and owning a DedicatedMemoryAllocator object
	 */
	class ResourceAllocatorDedicated : public ResourceAllocator
	{
	public:
		ResourceAllocatorDedicated() = default;
		ResourceAllocatorDedicated(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize stagingBlockSize = NVVK_DEFAULT_STAGING_BLOCKSIZE);
		virtual ~ResourceAllocatorDedicated();

		void init(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize stagingBlockSize = NVVK_DEFAULT_STAGING_BLOCKSIZE);
		// Provided such that ResourceAllocatorDedicated, ResourceAllocatorDma and ResourceAllocatorVma all have the same interface
		void init(VkInstance, VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize stagingBlockSize = NVVK_DEFAULT_STAGING_BLOCKSIZE);

		void deinit();

	protected:
		std::unique_ptr<MemAllocator> m_memAlloc;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	/**
	 #class nvvk::ExportResourceAllocator

	 ExportResourceAllocator specializes the object allocation process such that resulting memory allocations are
	 exportable and buffers and images can be bound to external memory.
	*/
	class ExportResourceAllocator : public ResourceAllocator
	{
	public:
		ExportResourceAllocator() = default;
		ExportResourceAllocator(VkDevice         device,
			VkPhysicalDevice physicalDevice,
			MemAllocator* memAlloc,
			VkDeviceSize     stagingBlockSize = NVVK_DEFAULT_STAGING_BLOCKSIZE);

	protected:
		virtual MemHandle AllocateMemory(const MemAllocateInfo& allocateInfo) override;
		virtual void      CreateBufferEx(const VkBufferCreateInfo& info_, VkBuffer* buffer) override;
		virtual void      CreateImageEx(const VkImageCreateInfo& info_, VkImage* image) override;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	/**
	 \class nvvk::ExportResourceAllocatorDedicated
	 nvvk::ExportResourceAllocatorDedicated is a resource allocator that is using DedicatedMemoryAllocator to allocate memory
	 and at the same time it'll make all allocations exportable.
	*/
	class ExportResourceAllocatorDedicated : public ExportResourceAllocator
	{
	public:
		ExportResourceAllocatorDedicated() = default;
		ExportResourceAllocatorDedicated(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize stagingBlockSize = NVVK_DEFAULT_STAGING_BLOCKSIZE);
		virtual ~ExportResourceAllocatorDedicated() override;

		void init(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize stagingBlockSize = NVVK_DEFAULT_STAGING_BLOCKSIZE);
		void deinit();

	protected:
		std::unique_ptr<MemAllocator> m_memAlloc;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	/**
	 \class nvvk::ExplicitDeviceMaskResourceAllocator
	 nvvk::ExplicitDeviceMaskResourceAllocator is a resource allocator that will inject a specific devicemask into each
	 allocation, making the created allocations and objects available to only the devices in the mask.
	*/
	class ExplicitDeviceMaskResourceAllocator : public ResourceAllocator
	{
	public:
		ExplicitDeviceMaskResourceAllocator() = default;
		ExplicitDeviceMaskResourceAllocator(VkDevice device, VkPhysicalDevice physicalDevice, MemAllocator* memAlloc, uint32_t deviceMask);

		void init(VkDevice device, VkPhysicalDevice physicalDevice, MemAllocator* memAlloc, uint32_t deviceMask);

	protected:
		virtual MemHandle AllocateMemory(const MemAllocateInfo& allocateInfo) override;

		uint32_t m_deviceMask;
	};

}  // namespace bgfx
