#pragma once

#ifdef LP_DEBUG
	#define LP_DEBUGBREAK() __debugbreak()
#else
	#define LP_DEBUGBREAK();
#endif

#ifdef LP_ENABLE_ASSERTS
	#define LP_ASSERT(x, ...) { if(!(x)) { LP_ERROR("Assertion Failed: {0}", __VA_ARGS__); LP_DEBUGBREAK(); } }
	#define LP_CORE_ASSERT(x, ...) { if(!(x)) { LP_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); LP_DEBUGBREAK(); } }
#else
	#define LP_ASSERT(x, ...)
	#define LP_CORE_ASSERT(x, ...)
#endif