#pragma once
#include <map>

namespace bgfx {

	enum EVkFunctionName {
		vkEnumeratePhysicalDevices,
		vkGetDeviceQueue,
		vkCreatePipelineCache,
		vkCreateCommandPool,
		vkGetPhysicalDeviceFeatures
	};

	class FunctionMapVk
	{
	private:
		static FunctionMapVk* m_instance;

	public:
		static FunctionMapVk* Get();

		void setup(std::map<EVkFunctionName, void*>& funcMap);

		void* getFunction(EVkFunctionName funcName);

	public:
		std::map<EVkFunctionName, void*> m_funcMap;
	};

}
