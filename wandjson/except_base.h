#ifndef _WANDJSON_EXCEPT_BASE_H_
#define _WANDJSON_EXCEPT_BASE_H_

#include "basedefs.h"
#include <peff/base/alloc.h>

namespace wandjson {
	enum class ErrorKind {
		OutOfMemory = 0,
		SyntaxError
	};

	class InternalException {
	public:
		mutable peff::RcObjectPtr<peff::Alloc> allocator;
		ErrorKind kind;

		WANDJSON_API InternalException(peff::Alloc *allocator, ErrorKind kind);
		WANDJSON_API virtual ~InternalException();

		virtual const char *what() const = 0;

		virtual void dealloc() = 0;
	};

	class InternalExceptionPointer {
	private:
		InternalException *_ptr = nullptr;

	public:
		WANDJSON_FORCEINLINE InternalExceptionPointer() noexcept = default;
		WANDJSON_FORCEINLINE InternalExceptionPointer(InternalException *exception) noexcept : _ptr(exception) {
		}

		WANDJSON_FORCEINLINE ~InternalExceptionPointer() noexcept {
			unwrap();
			reset();
		}

		InternalExceptionPointer(const InternalExceptionPointer &) = delete;
		InternalExceptionPointer &operator=(const InternalExceptionPointer &) = delete;
		WANDJSON_FORCEINLINE InternalExceptionPointer(InternalExceptionPointer &&other) noexcept {
			_ptr = other._ptr;
			other._ptr = nullptr;
		}
		WANDJSON_FORCEINLINE InternalExceptionPointer &operator=(InternalExceptionPointer &&other) noexcept {
			_ptr = other._ptr;
			other._ptr = nullptr;
			return *this;
		}

		WANDJSON_FORCEINLINE InternalException *get() noexcept {
			return _ptr;
		}
		WANDJSON_FORCEINLINE const InternalException *get() const noexcept {
			return _ptr;
		}

		WANDJSON_FORCEINLINE void reset() noexcept {
			if (_ptr) {
				_ptr->dealloc();
			}
			_ptr = nullptr;
		}

		WANDJSON_FORCEINLINE void unwrap() noexcept {
			if (_ptr) {
				assert(("Unhandled WandXML internal exception: ", false));
			}
		}

		WANDJSON_FORCEINLINE explicit operator bool() noexcept {
			return (bool)_ptr;
		}

		WANDJSON_FORCEINLINE InternalException *operator->() noexcept {
			return _ptr;
		}

		WANDJSON_FORCEINLINE const InternalException *operator->() const noexcept {
			return _ptr;
		}
	};
}

#define WANDJSON_UNWRAP_EXCEPT(expr) (expr).unwrap()
#define WANDJSON_RETURN_IF_EXCEPT(expr)                         \
	if (wandjson::InternalExceptionPointer e = (expr); (bool)e) \
	return e
#define WANDJSON_RETURN_IF_EXCEPT_WITH_LVAR(name, expr) \
	if ((bool)(name = (expr)))                         \
		return name;

#endif
