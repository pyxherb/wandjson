#ifndef _WANDJSON_EXCEPT_H_
#define _WANDJSON_EXCEPT_H_

#include "except_base.h"
#include <new>

namespace wandjson {
	/// @brief The out of memory error, indicates that a memory allocation has failed.
	class OutOfMemoryError : public InternalException {
	public:
		WANDJSON_API OutOfMemoryError();
		WANDJSON_API virtual ~OutOfMemoryError();

		WANDJSON_API virtual const char *what() const override;

		WANDJSON_API virtual void dealloc() override;

		WANDJSON_API static OutOfMemoryError *alloc();
	};

	extern OutOfMemoryError g_outOfMemoryError;

	class SyntaxError : public InternalException {
	public:
		const char *message;
		size_t off;

		WANDJSON_API SyntaxError(peff::Alloc *allocator, size_t off, const char *message);
		WANDJSON_API virtual ~SyntaxError();

		WANDJSON_API virtual const char *what() const override;

		WANDJSON_API virtual void dealloc() override;

		WANDJSON_API static SyntaxError *alloc(peff::Alloc *allocator, size_t off, const char *message) noexcept;
	};

	WANDJSON_FORCEINLINE InternalExceptionPointer withOutOfMemoryErrorIfAllocFailed(InternalException *exceptionPtr) noexcept {
		if (!exceptionPtr) {
			return OutOfMemoryError::alloc();
		}
		return exceptionPtr;
	}
}

#endif
