#include "value.h"

using namespace wandjson;

WANDJSON_API void ValueDestructionInfo::push_destructible(Value *node) {
	node->next_destructible = destructible_list;
	destructible_list = node;
}

WANDJSON_API Value::Value(ValueType node_type, peff::Alloc *allocator) : _allocator(allocator), _value_type(node_type) {
}

WANDJSON_API void Value::dealloc() {
	ValueDestructionInfo destruction_info;

	assert(!this->destruction_info);

	dealloc(destruction_info);

	Value *next;

	while ((next = destruction_info.destructible_list)) {
		destruction_info.destructible_list = nullptr;
		do {
			Value *cur = next;
			next = next->next_destructible;
			cur->dealloc(destruction_info);
		} while (next);
	}
}

WANDJSON_API NumberValue::NumberValue(peff::Alloc *allocator, int64_t data) : Value(ValueType::Number, allocator), _numberKind(NumberKind::Integer) {
	this->_data.as_integer = data;
}

WANDJSON_API NumberValue::NumberValue(peff::Alloc *allocator, double data) : Value(ValueType::Number, allocator), _numberKind(NumberKind::Decimal) {
	this->_data.as_decimal = data;
}

WANDJSON_API NumberValue::~NumberValue() {
}

WANDJSON_API void NumberValue::dealloc(ValueDestructionInfo &destruction_info) noexcept {
	peff::destroy_and_release<NumberValue>(get_allocator(), this, sizeof(std::max_align_t));
}

WANDJSON_API NumberValue *NumberValue::alloc(peff::Alloc *allocator, int64_t data) noexcept {
	return peff::alloc_and_construct<NumberValue>(allocator, sizeof(std::max_align_t), allocator, data);
}

WANDJSON_API NumberValue *NumberValue::alloc(peff::Alloc *allocator, double data) noexcept {
	return peff::alloc_and_construct<NumberValue>(allocator, sizeof(std::max_align_t), allocator, data);
}

WANDJSON_API StringValue::StringValue(peff::Alloc *allocator, peff::String &&data) : Value(ValueType::String, allocator), _data(std::move(data)) {
}

WANDJSON_API StringValue::~StringValue() {
}

WANDJSON_API void StringValue::dealloc(ValueDestructionInfo &destruction_info) noexcept {
	peff::destroy_and_release<StringValue>(get_allocator(), this, sizeof(std::max_align_t));
}

WANDJSON_API StringValue *StringValue::alloc(peff::Alloc *allocator, peff::String &&data) noexcept {
	return peff::alloc_and_construct<StringValue>(allocator, sizeof(std::max_align_t), allocator, std::move(data));
}

WANDJSON_API ArrayValue::ArrayValue(peff::Alloc *allocator) : Value(ValueType::Array, allocator), _data(allocator) {
}

WANDJSON_API ArrayValue::~ArrayValue() {
	for (auto i : this->_data) {
		this->destruction_info->push_destructible(i);
	}
}

WANDJSON_API void ArrayValue::dealloc(ValueDestructionInfo &destruction_info) noexcept {
	this->destruction_info = &destruction_info;

	peff::destroy_and_release<ArrayValue>(get_allocator(), this, sizeof(std::max_align_t));
}

WANDJSON_API ArrayValue *ArrayValue::alloc(peff::Alloc *allocator) noexcept {
	return peff::alloc_and_construct<ArrayValue>(allocator, sizeof(std::max_align_t), allocator);
}

WANDJSON_API ObjectFieldWrapper::ObjectFieldWrapper(ObjectFieldWrapper &&rhs) : parent(rhs.parent), name(std::move(rhs.name)), value(rhs.value) {
	rhs.parent = nullptr;
	rhs.value = nullptr;
}

WANDJSON_API ObjectFieldWrapper &ObjectFieldWrapper::operator=(ObjectFieldWrapper &&rhs) {
	parent = rhs.parent;
	value = rhs.value;
	rhs.parent = nullptr;
	rhs.name = nullptr;
	rhs.value = nullptr;
	return *this;
}

WANDJSON_API ObjectFieldWrapper::~ObjectFieldWrapper() {
	if (value)
		parent->destruction_info->push_destructible(value);
}

WANDJSON_API ObjectValue::ObjectValue(peff::Alloc *allocator) : Value(ValueType::Object, allocator), _data(allocator) {
}

WANDJSON_API ObjectValue::~ObjectValue() {
	// All destructibles are pushed by the wrapper destructor.
	for (auto i : _data)
		peff::destroy_and_release<ObjectFieldWrapper>(get_allocator(), i.second, alignof(ObjectFieldWrapper));
	_data.clear();
}

WANDJSON_API void ObjectValue::dealloc(ValueDestructionInfo &destruction_info) noexcept {
	this->destruction_info = &destruction_info;

	peff::destroy_and_release<ObjectValue>(get_allocator(), this, sizeof(std::max_align_t));
}

WANDJSON_API ObjectValue *ObjectValue::alloc(peff::Alloc *allocator) noexcept {
	return peff::alloc_and_construct<ObjectValue>(allocator, sizeof(std::max_align_t), allocator);
}

WANDJSON_API BooleanValue::BooleanValue(peff::Alloc *allocator, bool data) : Value(ValueType::Boolean, allocator), _data(std::move(data)) {
}

WANDJSON_API BooleanValue::~BooleanValue() {
}

WANDJSON_API void BooleanValue::dealloc(ValueDestructionInfo &destruction_info) noexcept {
	peff::destroy_and_release<BooleanValue>(get_allocator(), this, sizeof(std::max_align_t));
}

WANDJSON_API BooleanValue *BooleanValue::alloc(peff::Alloc *allocator, bool data) noexcept {
	return peff::alloc_and_construct<BooleanValue>(allocator, sizeof(std::max_align_t), allocator, data);
}
