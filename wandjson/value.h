#ifndef _WANDJSON_DOM_H_
#define _WANDJSON_DOM_H_

#include "except.h"
#include <peff/containers/string.h>
#include <peff/containers/hashmap.h>
#include <peff/containers/dynarray.h>
#include <optional>

namespace wandjson {
	enum class ValueType : uint8_t {
		Number,
		String,
		Array,
		Object,
		Boolean,
		Null
	};

	class Value;

	struct ValueDestructionInfo {
		Value *destructibleValueList = nullptr;

		WANDJSON_API void pushDestructible(Value *astNode);
	};

	struct ObjectFieldWrapper;

	class Value {
	private:
		ValueType _valueType;
		peff::RcObjectPtr<peff::Alloc> _allocator;

	protected:
		ValueDestructionInfo *destructionInfo = nullptr;
		Value *nextDestructible = nullptr;

		friend struct ObjectFieldWrapper;
		friend struct ValueDestructionInfo;

	public:
		WANDJSON_API Value(ValueType nodeType, peff::Alloc *allocator);

		WANDJSON_API void dealloc();
		virtual void dealloc(ValueDestructionInfo &destructionInfo) = 0;

		WANDJSON_FORCEINLINE ValueType getValueType() const noexcept {
			return _valueType;
		}

		WANDJSON_FORCEINLINE peff::Alloc *getAllocator() const noexcept {
			return _allocator.get();
		}
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
	private:
		union {
			int64_t asInteger;
			double asDecimal;
		} _data;
		NumberKind _numberKind;

	public:
		WANDJSON_API NumberValue(peff::Alloc *allocator, int64_t data);
		WANDJSON_API NumberValue(peff::Alloc *allocator, double data);
		WANDJSON_API virtual ~NumberValue();

		WANDJSON_API virtual void dealloc(ValueDestructionInfo &destructionInfo) noexcept override;

		WANDJSON_API static NumberValue *alloc(peff::Alloc *allocator, int64_t data) noexcept;
		WANDJSON_API static NumberValue *alloc(peff::Alloc *allocator, double data) noexcept;

		WANDJSON_FORCEINLINE NumberKind getNumberKind() const {
			return _numberKind;
		}

		WANDJSON_FORCEINLINE int64_t &asInteger() {
			return _data.asInteger;
		}

		WANDJSON_FORCEINLINE const int64_t &asInteger() const {
			return _data.asInteger;
		}

		WANDJSON_FORCEINLINE double &asDecimal() {
			return _data.asDecimal;
		}

		WANDJSON_FORCEINLINE const double &asDecimal() const {
			return _data.asDecimal;
		}
	};

	class StringValue final : public Value {
	private:
		peff::String _data;

	public:
		WANDJSON_API StringValue(peff::Alloc *allocator, peff::String &&data);
		WANDJSON_API virtual ~StringValue();

		WANDJSON_API virtual void dealloc(ValueDestructionInfo &destructionInfo) noexcept override;

		WANDJSON_API static StringValue *alloc(peff::Alloc *allocator, peff::String &&data) noexcept;

		WANDJSON_FORCEINLINE peff::String &data() {
			return _data;
		}

		WANDJSON_FORCEINLINE const peff::String &data() const {
			return _data;
		}
	};

	class ArrayValue final : public Value {
	private:
		peff::DynArray<Value *> _data;

	public:
		WANDJSON_API ArrayValue(peff::Alloc *allocator);
		WANDJSON_API virtual ~ArrayValue();

		WANDJSON_API virtual void dealloc(ValueDestructionInfo &destructionInfo) noexcept override;

		WANDJSON_API static ArrayValue *alloc(peff::Alloc *allocator) noexcept;

		using Iterator = decltype(_data)::Iterator;
		using ConstIterator = decltype(_data)::ConstIterator;

		WANDJSON_FORCEINLINE Iterator begin() {
			return _data.begin();
		}

		WANDJSON_FORCEINLINE ConstIterator begin() const {
			return _data.begin();
		}

		WANDJSON_FORCEINLINE Iterator end() {
			return _data.begin();
		}

		WANDJSON_FORCEINLINE ConstIterator end() const {
			return _data.begin();
		}

		WANDJSON_FORCEINLINE const peff::DynArray<Value *> &data() const {
			return _data;
		}

		WANDJSON_FORCEINLINE peff::DynArray<Value *> &data() {
			return _data;
		}
	};

	class ObjectValue;

	struct ObjectFieldWrapper {
		ObjectValue *parent;
		peff::String *name = nullptr;
		Value *value = nullptr;

		WANDJSON_FORCEINLINE ObjectFieldWrapper(ObjectValue *parent, peff::String *name, Value *value) : parent(parent), name(name), value(value) {}
		WANDJSON_API ObjectFieldWrapper(ObjectFieldWrapper &&rhs);
		WANDJSON_API ~ObjectFieldWrapper();

		WANDJSON_API ObjectFieldWrapper &operator=(ObjectFieldWrapper &&rhs);

		WANDJSON_FORCEINLINE const Value &operator*() const {
			return *value;
		}

		WANDJSON_FORCEINLINE Value &operator*() {
			return *value;
		}

		WANDJSON_FORCEINLINE Value *operator->() const {
			return value;
		}
	};

	class ObjectValue final : public Value {
	private:
		peff::HashMap<std::string_view, ObjectFieldWrapper> _data;

	public:
		WANDJSON_API ObjectValue(peff::Alloc *allocator);
		WANDJSON_API virtual ~ObjectValue();

		WANDJSON_API virtual void dealloc(ValueDestructionInfo &destructionInfo) noexcept override;

		WANDJSON_API static ObjectValue *alloc(peff::Alloc *allocator) noexcept;

		[[nodiscard]] WANDJSON_FORCEINLINE bool insert(peff::String &&name, Value *value) {
			peff::String *s = peff::allocAndConstruct<peff::String>(getAllocator(), alignof(peff::String), std::move(name));
			if (!s)
				return false;
			if (!_data.insert(*s, ObjectFieldWrapper(this, s, nullptr)))
				return false;
			_data.at(*s).value = value;
			return true;
		}

		[[nodiscard]] WANDJSON_FORCEINLINE bool remove(const std::string_view &name) {
			if (!_data.remove(name))
				return false;
			return true;
		}

		Value *at(const std::string_view &name) const {
			return _data.at(name).value;
		}

		Value *find(const std::string_view &name) const {
			if (auto it = _data.find(name); it != _data.end())
				return it.value().value;
			return nullptr;
		}

		using Iterator = decltype(_data)::Iterator;
		using ConstIterator = decltype(_data)::ConstIterator;

		WANDJSON_FORCEINLINE Iterator begin() {
			return _data.begin();
		}

		WANDJSON_FORCEINLINE ConstIterator begin() const {
			return _data.begin();
		}

		WANDJSON_FORCEINLINE Iterator end() {
			return _data.end();
		}

		WANDJSON_FORCEINLINE ConstIterator end() const {
			return _data.end();
		}
	};

	class BooleanValue final : public Value {
	private:
		bool _data;

	public:
		WANDJSON_API BooleanValue(peff::Alloc *allocator, bool data);
		WANDJSON_API virtual ~BooleanValue();

		WANDJSON_API virtual void dealloc(ValueDestructionInfo &destructionInfo) noexcept override;

		WANDJSON_API static BooleanValue *alloc(peff::Alloc *allocator, bool data) noexcept;

		WANDJSON_FORCEINLINE bool &data() {
			return _data;
		}

		WANDJSON_FORCEINLINE const bool &data() const {
			return _data;
		}
	};

	class NullValue final : public Value {
	public:
		WANDJSON_API NullValue(peff::Alloc *allocator);
		WANDJSON_API virtual ~NullValue();

		WANDJSON_API virtual void dealloc(ValueDestructionInfo &destructionInfo) noexcept override;

		WANDJSON_API static NullValue *alloc(peff::Alloc *allocator) noexcept;
	};
}

#endif
