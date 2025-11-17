#include "value.h"

using namespace wandjson;

WANDJSON_API void ValueDestructionInfo::pushDestructible(Value *astNode) {
	astNode->nextDestructible = destructibleValueList;
	destructibleValueList = astNode;
}

WANDJSON_API Value::Value(ValueType valueType, peff::Alloc *allocator) : _allocator(allocator), _valueType(valueType) {
}

WANDJSON_API void Value::dealloc() {
	ValueDestructionInfo destructionInfo;

	assert(!this->destructionInfo);

	dealloc(destructionInfo);

	Value *next;

	while ((next = destructionInfo.destructibleValueList)) {
		destructionInfo.destructibleValueList = nullptr;
		do {
			Value *cur = next;
			next = next->nextDestructible;
			cur->dealloc(destructionInfo);
		} while (next);
	}
}

WANDJSON_API NumberValue::NumberValue(peff::Alloc *allocator, int64_t data) : Value(ValueType::Number, allocator), _numberKind(NumberKind::Integer) {
	this->_data.asInteger = data;
}

WANDJSON_API NumberValue::NumberValue(peff::Alloc *allocator, double data) : Value(ValueType::Number, allocator), _numberKind(NumberKind::Decimal) {
	this->_data.asDecimal = data;
}

WANDJSON_API NumberValue::~NumberValue() {
}

WANDJSON_API void NumberValue::dealloc(ValueDestructionInfo &destructionInfo) noexcept {
	peff::destroyAndRelease<NumberValue>(getAllocator(), this, sizeof(std::max_align_t));
}

WANDJSON_API NumberValue *NumberValue::alloc(peff::Alloc *allocator, int64_t data) noexcept {
	return peff::allocAndConstruct<NumberValue>(allocator, sizeof(std::max_align_t), allocator, data);
}

WANDJSON_API NumberValue *NumberValue::alloc(peff::Alloc *allocator, double data) noexcept {
	return peff::allocAndConstruct<NumberValue>(allocator, sizeof(std::max_align_t), allocator, data);
}

WANDJSON_API StringValue::StringValue(peff::Alloc *allocator, peff::String &&data) : Value(ValueType::String, allocator), _data(std::move(data)) {
}

WANDJSON_API StringValue::~StringValue() {
}

WANDJSON_API void StringValue::dealloc(ValueDestructionInfo &destructionInfo) noexcept {
	peff::destroyAndRelease<StringValue>(getAllocator(), this, sizeof(std::max_align_t));
}

WANDJSON_API StringValue *StringValue::alloc(peff::Alloc *allocator, peff::String &&data) noexcept {
	return peff::allocAndConstruct<StringValue>(allocator, sizeof(std::max_align_t), allocator, std::move(data));
}

WANDJSON_API ArrayValue::ArrayValue(peff::Alloc *allocator) : Value(ValueType::Array, allocator), _data(allocator) {
}

WANDJSON_API ArrayValue::~ArrayValue() {
	for (auto i : this->_data) {
		this->destructionInfo->pushDestructible(i);
	}
}

WANDJSON_API void ArrayValue::dealloc(ValueDestructionInfo &destructionInfo) noexcept {
	this->destructionInfo = &destructionInfo;

	peff::destroyAndRelease<ArrayValue>(getAllocator(), this, sizeof(std::max_align_t));
}

WANDJSON_API ArrayValue *ArrayValue::alloc(peff::Alloc *allocator) noexcept {
	return peff::allocAndConstruct<ArrayValue>(allocator, sizeof(std::max_align_t), allocator);
}

WANDJSON_API ObjectFieldWrapper::ObjectFieldWrapper(ObjectFieldWrapper &&rhs) : parent(rhs.parent), name(rhs.name), value(rhs.value){
	rhs.parent = nullptr;
	rhs.name = nullptr;
	rhs.value = nullptr;
}

WANDJSON_API ObjectFieldWrapper &ObjectFieldWrapper::operator = (ObjectFieldWrapper &&rhs) {
	parent = rhs.parent;
	name = rhs.name;
	value = rhs.value;
	rhs.parent = nullptr;
	rhs.name = nullptr;
	rhs.value = nullptr;
	return *this;
}

WANDJSON_API ObjectFieldWrapper::~ObjectFieldWrapper() {
	if (name)
		peff::destroyAndRelease<peff::String>(parent->getAllocator(), name, alignof(peff::String));

	if (value)
		parent->destructionInfo->pushDestructible(value);
}

WANDJSON_API ObjectValue::ObjectValue(peff::Alloc *allocator) : Value(ValueType::Object, allocator), _data(allocator) {
}

WANDJSON_API ObjectValue::~ObjectValue() {
	// All destructibles are pushed by the wrapper destructor.
	_data.clear();
}

WANDJSON_API void ObjectValue::dealloc(ValueDestructionInfo &destructionInfo) noexcept {
	this->destructionInfo = &destructionInfo;

	peff::destroyAndRelease<ObjectValue>(getAllocator(), this, sizeof(std::max_align_t));
}

WANDJSON_API ObjectValue *ObjectValue::alloc(peff::Alloc *allocator) noexcept {
	return peff::allocAndConstruct<ObjectValue>(allocator, sizeof(std::max_align_t), allocator);
}

WANDJSON_API BooleanValue::BooleanValue(peff::Alloc *allocator, bool data) : Value(ValueType::Boolean, allocator), _data(std::move(data)) {
}

WANDJSON_API BooleanValue::~BooleanValue() {
}

WANDJSON_API void BooleanValue::dealloc(ValueDestructionInfo &destructionInfo) noexcept {
	peff::destroyAndRelease<BooleanValue>(getAllocator(), this, sizeof(std::max_align_t));
}

WANDJSON_API BooleanValue *BooleanValue::alloc(peff::Alloc *allocator, bool data) noexcept {
	return peff::allocAndConstruct<BooleanValue>(allocator, sizeof(std::max_align_t), allocator, data);
}

WANDJSON_API NullValue::NullValue(peff::Alloc *allocator) : Value(ValueType::Null, allocator) {
}

WANDJSON_API NullValue::~NullValue() {
}

WANDJSON_API void NullValue::dealloc(ValueDestructionInfo &destructionInfo) noexcept {
	peff::destroyAndRelease<NullValue>(getAllocator(), this, sizeof(std::max_align_t));
}

WANDJSON_API NullValue *NullValue::alloc(peff::Alloc *allocator) noexcept {
	return peff::allocAndConstruct<NullValue>(allocator, sizeof(std::max_align_t), allocator);
}
