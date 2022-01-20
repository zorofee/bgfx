#pragma once
namespace bgfx {


	// Objects
	struct Buffer
	{
		VkBuffer  buffer = VK_NULL_HANDLE;
		//MemHandle memHandle{ nullptr };
	};

	struct Image
	{
		VkImage   image = VK_NULL_HANDLE;
		//MemHandle memHandle{ nullptr };
	};

	struct Texture
	{
		VkImage               image = VK_NULL_HANDLE;
		//MemHandle             memHandle{ nullptr };
		VkDescriptorImageInfo descriptor{};
	};

	struct AccelNV
	{
		VkAccelerationStructureNV accel = VK_NULL_HANDLE;
		//MemHandle                 memHandle{ nullptr };
	};

	struct AccelKHR
	{
		VkAccelerationStructureKHR accel = VK_NULL_HANDLE;
		Buffer               buffer;
	};


	class ResourceAllocator {


	};
}
