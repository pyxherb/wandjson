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
		Boolean
	};

	class Value;

	struct ValueDestructionInfo {
		Value *destructible_list = nullptr;

		WANDJSON_API void push_destructible(Value *node);
	};

	struct ObjectFieldWrapper;

	class Value {
	private:
		ValueType _value_type;
		peff::RcObjectPtr<peff::Alloc> _allocator;

	protected:
		ValueDestructionInfo *destruction_info = nullptr;
		Value *next_destructible = nullptr;

		friend struct ObjectFieldWrapper;
		friend struct ValueDestructionInfo;

	public:
		WANDJSON_API Value(ValueType node_type, peff::Alloc *allocator);

		WANDJSON_API void dealloc();
		virtual void dealloc(ValueDestructionInfo &destruction_info) = 0;

		WANDJSON_FORCEINLINE ValueType get_value_type() const noexcept {
			return _value_type;
		}

		WANDJSON_FORCEINLINE peff::Alloc *get_allocator() const noexcept {
			return _allocator.get();
		}

		WANDJSON_FORCEINLINE bool is_number() const noexcept {
			return get_value_type() == ValueType::Number;
		}

		WANDJSON_FORCEINLINE bool is_string() const noexcept {
			return get_value_type() == ValueType::String;
		}

		WANDJSON_FORCEINLINE bool is_array() const noexcept {
			return get_value_type() == ValueType::Array;
		}

		WANDJSON_FORCEINLINE bool is_object() const noexcept {
			return get_value_type() == ValueType::Object;
		}

		WANDJSON_FORCEINLINE bool is_boolean() const noexcept {
			return get_value_type() == ValueType::Boolean;
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
			int64_t as_integer;
			double as_decimal;
		} _data;
		NumberKind _numberKind;

	public:
		WANDJSON_API NumberValue(peff::Alloc *allocator, int64_t data);
		WANDJSON_API NumberValue(peff::Alloc *allocator, double data);
		WANDJSON_API virtual ~NumberValue();

		WANDJSON_API virtual void dealloc(ValueDestructionInfo &destruction_info) noexcept override;

		WANDJSON_API static NumberValue *alloc(peff::Alloc *allocator, int64_t data) noexcept;
		WANDJSON_API static NumberValue *alloc(peff::Alloc *allocator, double data) noexcept;

		WANDJSON_FORCEINLINE NumberKind get_number_type() const {
			return _numberKind;
		}

		WANDJSON_FORCEINLINE int64_t &as_integer() {
			return _data.as_integer;
		}

		WANDJSON_FORCEINLINE const int64_t &as_integer() const {
			return _data.as_integer;
		}

		WANDJSON_FORCEINLINE double &as_decimal() {
			return _data.as_decimal;
		}

		WANDJSON_FORCEINLINE const double &as_decimal() const {
			return _data.as_decimal;
		}
	};

	class StringValue final : public Value {
	private:
		peff::String _data;

	public:
		WANDJSON_API StringValue(peff::Alloc *allocator, peff::String &&data);
		WANDJSON_API virtual ~StringValue();

		WANDJSON_API virtual void dealloc(ValueDestructionInfo &destruction_info) noexcept override;

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

		WANDJSON_API virtual void dealloc(ValueDestructionInfo &destruction_info) noexcept override;

		WANDJSON_API static ArrayValue *alloc(peff::Alloc *allocator) noexcept;

		using Iterator = decltype(_data)::Iterator;
		using ConstIterator = decltype(_data)::ConstIterator;

		WANDJSON_FORCEINLINE Iterator begin() noexcept {
			return _data.begin();
		}

		WANDJSON_FORCEINLINE ConstIterator begin() const noexcept {
			return _data.begin();
		}

		WANDJSON_FORCEINLINE Iterator end() noexcept {
			return _data.end();
		}

		WANDJSON_FORCEINLINE ConstIterator end() const noexcept {
			return _data.end();
		}

		WANDJSON_FORCEINLINE bool push_back(Value *value) noexcept {
			return _data.push_back(+value);
		}

		WANDJSON_FORCEINLINE bool push_front(Value *value) noexcept {
			return _data.push_front(+value);
		}

		WANDJSON_FORCEINLINE const peff::DynArray<Value *> &data() const noexcept {
			return _data;
		}

		WANDJSON_FORCEINLINE peff::DynArray<Value *> &data() noexcept {
			return _data;
		}
	};

	class ObjectValue;

	struct ObjectFieldWrapper {
		ObjectValue *parent;
		peff::String name = nullptr;
		Value *value = nullptr;

		WANDJSON_FORCEINLINE ObjectFieldWrapper(ObjectValue *parent, peff::String &&name, Value *value) : parent(parent), name(std::move(name)), value(value) {}
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
		peff::HashMap<std::string_view, ObjectFieldWrapper *> _data;

	public:
		WANDJSON_API ObjectValue(peff::Alloc *allocator);
		WANDJSON_API virtual ~ObjectValue();

		WANDJSON_API virtual void dealloc(ValueDestructionInfo &destruction_info) noexcept override;

		WANDJSON_API static ObjectValue *alloc(peff::Alloc *allocator) noexcept;

		/// @brief Insert a value. Note that the value will not be deleted if the insertion was failed.
		/// @param name Name for insertion.
		/// @param value Value to be inserted.
		/// @return Whether the value is successfully inserted.
		[[nodiscard]] WANDJSON_FORCEINLINE bool insert(peff::String &&name, Value *value) {
			ObjectFieldWrapper *wrapper = peff::alloc_and_construct<ObjectFieldWrapper>(get_allocator(), alignof(ObjectFieldWrapper), this, std::move(name), value);
			if (!wrapper)
				return false;
			peff::ScopeGuard sg([this, wrapper]() noexcept {
				peff::destroy_and_release<ObjectFieldWrapper>(get_allocator(), wrapper, alignof(ObjectFieldWrapper));
			});
			if (!_data.insert(wrapper->name, +wrapper))
				return false;
			sg.release();
			return true;
		}

		WANDJSON_FORCEINLINE void remove(const std::string_view &name) {
			peff::destroy_and_release<ObjectFieldWrapper>(get_allocator(), _data.at(name), alignof(ObjectFieldWrapper));
			_data.remove(name);
		}

		WANDJSON_FORCEINLINE Value *at(const std::string_view &name) const {
			return _data.at(name)->value;
		}

		WANDJSON_FORCEINLINE Value *find(const std::string_view &name) const {
			if (auto it = _data.find(name); it != _data.end())
				return it.value()->value;
			return nullptr;
		}

		struct Iterator {
			decltype(_data)::Iterator _iterator;

			WANDJSON_FORCEINLINE Iterator(const decltype(_data)::Iterator &iterator) : _iterator(iterator) {}
			WANDJSON_FORCEINLINE Iterator(decltype(_data)::Iterator &&iterator) : _iterator(std::move(iterator)) {}

			WANDJSON_FORCEINLINE Iterator(const Iterator &) = default;
			WANDJSON_FORCEINLINE Iterator(Iterator &&) = default;
			WANDJSON_FORCEINLINE Iterator &operator=(const Iterator &) = default;
			WANDJSON_FORCEINLINE Iterator &operator=(Iterator &&) = default;

			WANDJSON_FORCEINLINE std::string_view key() const {
				return _iterator.key();
			}

			WANDJSON_FORCEINLINE Value *value() const {
				return _iterator.value()->value;
			}

			PEFF_FORCEINLINE std::pair<std::string_view, Value *> operator*() const {
				return { key(), value() };
			}

			PEFF_FORCEINLINE bool operator==(const Iterator &rhs) const {
				return _iterator == rhs._iterator;
			}

			PEFF_FORCEINLINE bool operator==(Iterator &&rhs) const {
				return _iterator == rhs._iterator;
			}

			PEFF_FORCEINLINE bool operator!=(const Iterator &rhs) const {
				return _iterator != rhs._iterator;
			}

			PEFF_FORCEINLINE bool operator!=(Iterator &&rhs) const {
				return _iterator != rhs._iterator;
			}

			PEFF_FORCEINLINE Iterator &operator++() {
				++_iterator;
				return *this;
			}

			PEFF_FORCEINLINE Iterator operator++(int) {
				Iterator it = *this;
				++*this;
				return it;
			}
		};

		struct ConstIterator {
			decltype(_data)::ConstIterator _iterator;

			WANDJSON_FORCEINLINE ConstIterator(const decltype(_data)::ConstIterator &iterator) : _iterator(iterator) {}
			WANDJSON_FORCEINLINE ConstIterator(decltype(_data)::ConstIterator &&iterator) : _iterator(std::move(iterator)) {}

			WANDJSON_FORCEINLINE ConstIterator(const ConstIterator &) = default;
			WANDJSON_FORCEINLINE ConstIterator(ConstIterator &&) = default;
			WANDJSON_FORCEINLINE ConstIterator &operator=(const ConstIterator &) = default;
			WANDJSON_FORCEINLINE ConstIterator &operator=(ConstIterator &&) = default;

			WANDJSON_FORCEINLINE std::string_view key() const {
				return _iterator.key();
			}

			WANDJSON_FORCEINLINE Value *value() const {
				return _iterator.value()->value;
			}

			PEFF_FORCEINLINE std::pair<std::string_view, Value *> operator*() const {
				return { key(), value() };
			}

			PEFF_FORCEINLINE bool operator==(const ConstIterator &rhs) const {
				return _iterator == rhs._iterator;
			}

			PEFF_FORCEINLINE bool operator==(ConstIterator &&rhs) const {
				return _iterator == rhs._iterator;
			}

			PEFF_FORCEINLINE bool operator!=(const ConstIterator &rhs) const {
				return _iterator != rhs._iterator;
			}

			PEFF_FORCEINLINE bool operator!=(ConstIterator &&rhs) const {
				return _iterator != rhs._iterator;
			}

			PEFF_FORCEINLINE ConstIterator &operator++() {
				++_iterator;
				return *this;
			}

			PEFF_FORCEINLINE ConstIterator operator++(int) {
				ConstIterator it = *this;
				++*this;
				return it;
			}
		};

		WANDJSON_FORCEINLINE Iterator begin() {
			return Iterator(_data.begin());
		}

		WANDJSON_FORCEINLINE ConstIterator begin() const {
			return ConstIterator(_data.begin());
		}

		WANDJSON_FORCEINLINE ConstIterator begin_const() const {
			return ConstIterator(_data.begin());
		}

		WANDJSON_FORCEINLINE Iterator end() {
			return Iterator(_data.end());
		}

		WANDJSON_FORCEINLINE ConstIterator end() const {
			return ConstIterator(_data.end());
		}

		WANDJSON_FORCEINLINE ConstIterator end_const() const {
			return ConstIterator(_data.end());
		}
	};

	class BooleanValue final : public Value {
	private:
		bool _data;

	public:
		WANDJSON_API BooleanValue(peff::Alloc *allocator, bool data);
		WANDJSON_API virtual ~BooleanValue();

		WANDJSON_API virtual void dealloc(ValueDestructionInfo &destruction_info) noexcept override;

		WANDJSON_API static BooleanValue *alloc(peff::Alloc *allocator, bool data) noexcept;

		WANDJSON_FORCEINLINE bool &data() {
			return _data;
		}

		WANDJSON_FORCEINLINE const bool &data() const {
			return _data;
		}
	};

	WANDJSON_FORCEINLINE bool is_number(const Value *value) noexcept {
		return value ? value->get_value_type() == ValueType::Number : false;
	}

	WANDJSON_FORCEINLINE bool is_string(const Value *value) noexcept {
		return value ? value->get_value_type() == ValueType::String : false;
	}

	WANDJSON_FORCEINLINE bool is_array(const Value *value) noexcept {
		return value ? value->get_value_type() == ValueType::Array : false;
	}

	WANDJSON_FORCEINLINE bool is_object(const Value *value) noexcept {
		return value ? value->get_value_type() == ValueType::Object : false;
	}

	WANDJSON_FORCEINLINE bool is_boolean(const Value *value) noexcept {
		return value ? value->get_value_type() == ValueType::Boolean : false;
	}

	WANDJSON_FORCEINLINE bool to_number(Value *value, NumberValue *&value_out) noexcept {
		if (!value)
			return false;
		if (value->get_value_type() != ValueType::Number)
			return false;
		value_out = (NumberValue *)value;
		return true;
	}

	WANDJSON_FORCEINLINE bool to_string(Value *value, StringValue *&value_out) noexcept {
		if (!value)
			return false;
		if (value->get_value_type() != ValueType::String)
			return false;
		value_out = (StringValue *)value;
		return true;
	}

	WANDJSON_FORCEINLINE bool to_array(Value *value, ArrayValue *&value_out) noexcept {
		if (!value)
			return false;
		if (value->get_value_type() != ValueType::Array)
			return false;
		value_out = (ArrayValue *)value;
		return true;
	}

	WANDJSON_FORCEINLINE bool to_object(Value *value, ObjectValue *&value_out) noexcept {
		if (!value)
			return false;
		if (value->get_value_type() != ValueType::Object)
			return false;
		value_out = (ObjectValue *)value;
		return true;
	}

	WANDJSON_FORCEINLINE bool to_boolean(Value *value, BooleanValue *&value_out) noexcept {
		if (!value)
			return false;
		if (value->get_value_type() != ValueType::Boolean)
			return false;
		value_out = (BooleanValue *)value;
		return true;
	}
}

#endif
