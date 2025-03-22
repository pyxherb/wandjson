#ifndef _WANDJSON_DOM_H_
#define _WANDJSON_DOM_H_

#include "except.h"
#include <peff/containers/string.h>
#include <peff/containers/hashmap.h>
#include <peff/containers/dynarray.h>
#include <optional>

namespace wandjson {
	enum class NodeType : uint8_t {
		Number,
		String,
		Array,
		Object,
		Boolean,
		Null
	};

	class Value {
	public:
		peff::RcObjectPtr<peff::Alloc> allocator;
		NodeType nodeType;

		WANDJSON_API Value(NodeType nodeType, peff::Alloc *allocator);

		virtual void dealloc() = 0;
	};

	struct ValueDeleter {
		void operator()(Value *ptr) {
			if (ptr) {
				ptr->dealloc();
			}
		}
	};

	enum class NumberKind : uint8_t {
		Integer = 0,
		Decimal
	};

	class NumberValue final : public Value {
	public:
		union {
			uint64_t asInteger;
			double asDecimal;
		} data;
		NumberKind numberKind;

		WANDJSON_API NumberValue(peff::Alloc *allocator, uint64_t data);
		WANDJSON_API NumberValue(peff::Alloc *allocator, double data);
		WANDJSON_API virtual ~NumberValue();

		WANDJSON_API virtual void dealloc() noexcept override;

		WANDJSON_API static NumberValue *alloc(peff::Alloc *allocator, uint64_t data) noexcept;
		WANDJSON_API static NumberValue *alloc(peff::Alloc *allocator, double data) noexcept;

		WANDJSON_FORCEINLINE uint64_t getInteger() const {
			return data.asInteger;
		}

		WANDJSON_FORCEINLINE double asDecimal() const {
			return data.asDecimal;
		}
	};

	class StringValue final : public Value {
	public:
		peff::String data;

		WANDJSON_API StringValue(peff::Alloc *allocator, peff::String &&data);
		WANDJSON_API virtual ~StringValue();

		WANDJSON_API virtual void dealloc() noexcept override;

		WANDJSON_API static StringValue *alloc(peff::Alloc *allocator, peff::String &&data) noexcept;
	};

	class ArrayValue final : public Value {
	public:
		peff::DynArray<std::unique_ptr<Value, ValueDeleter>> data;

		WANDJSON_API ArrayValue(peff::Alloc *allocator);
		WANDJSON_API virtual ~ArrayValue();

		WANDJSON_API virtual void dealloc() noexcept override;

		WANDJSON_API static ArrayValue *alloc(peff::Alloc *allocator) noexcept;
	};

	class ObjectValue final : public Value {
	public:
		peff::HashMap<peff::String, std::unique_ptr<Value, ValueDeleter>> data;

		WANDJSON_API ObjectValue(peff::Alloc *allocator);
		WANDJSON_API virtual ~ObjectValue();

		WANDJSON_API virtual void dealloc() noexcept override;

		WANDJSON_API static ObjectValue *alloc(peff::Alloc *allocator) noexcept;
	};

	class BooleanValue final : public Value {
	public:
		bool data;

		WANDJSON_API BooleanValue(peff::Alloc *allocator, bool data);
		WANDJSON_API virtual ~BooleanValue();

		WANDJSON_API virtual void dealloc() noexcept override;

		WANDJSON_API static BooleanValue *alloc(peff::Alloc *allocator, bool data) noexcept;
	};

	class NullValue final : public Value {
	public:
		WANDJSON_API NullValue(peff::Alloc *allocator);
		WANDJSON_API virtual ~NullValue();

		WANDJSON_API virtual void dealloc() noexcept override;

		WANDJSON_API static NullValue *alloc(peff::Alloc *allocator) noexcept;
	};
}

#endif
