#include "value.h"

using namespace wandjson;

WANDJSON_API Value::Value(NodeType nodeType, peff::Alloc* allocator) : allocator(allocator), nodeType(nodeType) {
}

WANDJSON_API NumberValue::NumberValue(peff::Alloc *allocator, uint64_t data): Value(NodeType::Number, allocator), numberKind(NumberKind::Integer) {
	this->data.asInteger = data;
}

WANDJSON_API NumberValue::NumberValue(peff::Alloc *allocator, double data) : Value(NodeType::Number, allocator), numberKind(NumberKind::Decimal) {
	this->data.asDecimal = data;
}

WANDJSON_API NumberValue::~NumberValue() {
}

WANDJSON_API void NumberValue::dealloc() noexcept {
	peff::destroyAndRelease<NumberValue>(allocator.get(), this, sizeof(std::max_align_t));
}

WANDJSON_API NumberValue *NumberValue::alloc(peff::Alloc *allocator, uint64_t data) noexcept {
	return peff::allocAndConstruct<NumberValue>(allocator, sizeof(std::max_align_t), allocator, data);
}

WANDJSON_API NumberValue *NumberValue::alloc(peff::Alloc *allocator, double data) noexcept {
	return peff::allocAndConstruct<NumberValue>(allocator, sizeof(std::max_align_t), allocator, data);
}

WANDJSON_API StringValue::StringValue(peff::Alloc *allocator, peff::String &&data) : Value(NodeType::String, allocator), data(std::move(data)) {
}

WANDJSON_API StringValue::~StringValue() {
}

WANDJSON_API void StringValue::dealloc() noexcept {
	peff::destroyAndRelease<StringValue>(allocator.get(), this, sizeof(std::max_align_t));
}

WANDJSON_API StringValue *StringValue::alloc(peff::Alloc *allocator, peff::String &&data) noexcept {
	return peff::allocAndConstruct<StringValue>(allocator, sizeof(std::max_align_t), allocator, std::move(data));
}

WANDJSON_API ArrayValue::ArrayValue(peff::Alloc *allocator) : Value(NodeType::Array, allocator), data(allocator) {
}

WANDJSON_API ArrayValue::~ArrayValue() {
}

WANDJSON_API void ArrayValue::dealloc() noexcept {
	peff::destroyAndRelease<ArrayValue>(allocator.get(), this, sizeof(std::max_align_t));
}

WANDJSON_API ArrayValue *ArrayValue::alloc(peff::Alloc *allocator) noexcept {
	return peff::allocAndConstruct<ArrayValue>(allocator, sizeof(std::max_align_t), allocator);
}

WANDJSON_API ObjectValue::ObjectValue(peff::Alloc *allocator) : Value(NodeType::Object, allocator), data(allocator) {
}

WANDJSON_API ObjectValue::~ObjectValue() {
}

WANDJSON_API void ObjectValue::dealloc() noexcept {
	peff::destroyAndRelease<ObjectValue>(allocator.get(), this, sizeof(std::max_align_t));
}

WANDJSON_API ObjectValue *ObjectValue::alloc(peff::Alloc *allocator) noexcept {
	return peff::allocAndConstruct<ObjectValue>(allocator, sizeof(std::max_align_t), allocator);
}

WANDJSON_API BooleanValue::BooleanValue(peff::Alloc *allocator, bool data) : Value(NodeType::Boolean, allocator), data(std::move(data)) {
}

WANDJSON_API BooleanValue::~BooleanValue() {
}

WANDJSON_API void BooleanValue::dealloc() noexcept {
	peff::destroyAndRelease<BooleanValue>(allocator.get(), this, sizeof(std::max_align_t));
}

WANDJSON_API BooleanValue *BooleanValue::alloc(peff::Alloc *allocator, bool data) noexcept {
	return peff::allocAndConstruct<BooleanValue>(allocator, sizeof(std::max_align_t), allocator, data);
}

WANDJSON_API NullValue::NullValue(peff::Alloc *allocator) : Value(NodeType::Null, allocator) {
}

WANDJSON_API NullValue::~NullValue() {
}

WANDJSON_API void NullValue::dealloc() noexcept {
	peff::destroyAndRelease<NullValue>(allocator.get(), this, sizeof(std::max_align_t));
}

WANDJSON_API NullValue *NullValue::alloc(peff::Alloc *allocator) noexcept {
	return peff::allocAndConstruct<NullValue>(allocator, sizeof(std::max_align_t), allocator);
}
