#ifndef _WANDJSON_BASE_BASEDEFS_H_
#define _WANDJSON_BASE_BASEDEFS_H_

#include <peff/base/basedefs.h>

#define WANDJSON_DLLEXPORT PEFF_DLLEXPORT
#define WANDJSON_DLLIMPORT PEFF_DLLIMPORT
#define WANDJSON_FORCEINLINE PEFF_FORCEINLINE

#if defined(_MSC_VER)
	#define WANDJSON_DECL_EXPLICIT_INSTANTIATED_CLASS(api_modifier, name, ...) \
		api_modifier extern template class name<__VA_ARGS__>;
	#define WANDJSON_DEF_EXPLICIT_INSTANTIATED_CLASS(api_modifier, name, ...) \
		api_modifier template class name<__VA_ARGS__>;
#elif defined(__GNUC__) || defined(__clang__)
	#define WANDJSON_DECL_EXPLICIT_INSTANTIATED_CLASS(api_modifier, name, ...) \
		extern template class api_modifier name<__VA_ARGS__>;
	#define WANDJSON_DEF_EXPLICIT_INSTANTIATED_CLASS(api_modifier, name, ...) \
		template class name<__VA_ARGS__>;
#else
	#define WANDJSON_DECL_EXPLICIT_INSTANTIATED_CLASS(api_modifier, name, ...)
	#define WANDJSON_DEF_EXPLICIT_INSTANTIATED_CLASS(api_modifier, name, ...)
#endif

#if WANDJSON_DYNAMIC_LINK
	#if IS_WANDJSON_BASE_BUILDING
		#define WANDJSON_API WANDJSON_DLLEXPORT
	#else
		#define WANDJSON_API WANDJSON_DLLIMPORT
	#endif
#else
	#define WANDJSON_API
#endif

#endif
