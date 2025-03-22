#ifndef _WANDJSON_BASE_BASEDEFS_H_
#define _WANDJSON_BASE_BASEDEFS_H_

#include <peff/base/basedefs.h>

#if WANDJSON_DYNAMIC_LINK
	#if defined(_MSC_VER)
		#define WANDJSON_DLLEXPORT __declspec(dllexport)
		#define WANDJSON_DLLIMPORT __declspec(dllimport)
	#elif defined(__GNUC__) || defined(__clang__)
		#define WANDJSON_DLLEXPORT __attribute__((__visibility__("default")))
		#define WANDJSON_DLLIMPORT __attribute__((__visibility__("default")))
	#endif
#else
	#define WANDJSON_DLLEXPORT
	#define WANDJSON_DLLIMPORT
#endif

#define WANDJSON_FORCEINLINE PEFF_FORCEINLINE

#if defined(_MSC_VER)
	#define WANDJSON_DECL_EXPLICIT_INSTANTIATED_CLASS(apiModifier, name, ...) \
		apiModifier extern template class name<__VA_ARGS__>;
	#define WANDJSON_DEF_EXPLICIT_INSTANTIATED_CLASS(apiModifier, name, ...) \
		apiModifier template class name<__VA_ARGS__>;
#elif defined(__GNUC__) || defined(__clang__)
	#define WANDJSON_DECL_EXPLICIT_INSTANTIATED_CLASS(apiModifier, name, ...) \
		extern template class apiModifier name<__VA_ARGS__>;
	#define WANDJSON_DEF_EXPLICIT_INSTANTIATED_CLASS(apiModifier, name, ...) \
		template class name<__VA_ARGS__>;
#else
	#define WANDJSON_DECL_EXPLICIT_INSTANTIATED_CLASS(apiModifier, name, ...)
	#define WANDJSON_DEF_EXPLICIT_INSTANTIATED_CLASS(apiModifier, name, ...)
#endif

#if IS_WANDJSON_BASE_BUILDING
	#define WANDJSON_API WANDJSON_DLLEXPORT
#else
	#define WANDJSON_API WANDJSON_DLLIMPORT
#endif

#endif
