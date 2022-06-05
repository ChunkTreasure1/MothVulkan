#pragma once

#include <memory>

const char* VKResultToString(int32_t result);

#define BIT(X) (1 << (X))

#ifdef LP_DEBUG
	#define LP_DEBUGBREAK() __debugbreak()
	#define LP_VK_CHECK(x) if (x != VK_SUCCESS) { LP_CORE_ERROR("Vulkan Error: {0}", VKResultToString(x)); LP_DEBUGBREAK(); }
	#define LP_ENABLE_DEBUG_ALLOCATIONS
	#define LP_ENABLE_SHADER_DEBUG
#else

#ifdef LP_RELEASE
	#define LP_ENABLE_SHADER_DEBUG
#endif

	#define LP_DEBUGBREAK();
	#define LP_VK_CHECK(x) x
#endif

#ifdef LP_ENABLE_ASSERTS
	#define LP_ASSERT(x, ...) { if(!(x)) { LP_ERROR("Assertion Failed: {0}", __VA_ARGS__); LP_DEBUGBREAK(); } }
	#define LP_CORE_ASSERT(x, ...) { if(!(x)) { LP_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); LP_DEBUGBREAK(); } }
#else
	#define LP_ASSERT(x, ...)
	#define LP_CORE_ASSERT(x, ...)
#endif

#define LP_BIND_EVENT_FN(fn) std::bind(&fn, this, std::placeholders::_1)

template<typename T>
using Scope = std::unique_ptr<T>;

template<typename T, typename ... Args>
constexpr Scope<T> CreateScope(Args&& ... args)
{
	return std::make_unique<T>(std::forward<Args>(args)...);
}

template<typename T>
using Ref = std::shared_ptr<T>;

template<typename T, typename ... Args>
constexpr Ref<T> CreateRef(Args&& ... args)
{
	return std::make_shared<T>(std::forward<Args>(args)...);
}