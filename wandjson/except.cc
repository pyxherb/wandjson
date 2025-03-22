#include "except.h"

using namespace wandjson;

OutOfMemoryError wandjson::g_outOfMemoryError;

WANDJSON_API OutOfMemoryError::OutOfMemoryError() : InternalException(nullptr, ErrorKind::OutOfMemory) {}
WANDJSON_API OutOfMemoryError::~OutOfMemoryError() {}

WANDJSON_API const char *OutOfMemoryError::what() const {
	return "Out of memory";
}

WANDJSON_API void OutOfMemoryError::dealloc() {
}

WANDJSON_API OutOfMemoryError *OutOfMemoryError::alloc() {
	return &g_outOfMemoryError;
}

WANDJSON_API SyntaxError::SyntaxError(peff::Alloc *allocator, size_t off, const char *message)
	: InternalException(allocator, ErrorKind::SyntaxError),
	  off(off),
	  message(message) {}
WANDJSON_API SyntaxError::~SyntaxError() {}

WANDJSON_API const char *SyntaxError::what() const {
	return message;
}

WANDJSON_API void SyntaxError::dealloc() {
	this->~SyntaxError();
	allocator->release((void *)this, sizeof(SyntaxError), sizeof(std::max_align_t));
}

WANDJSON_API SyntaxError *SyntaxError::alloc(peff::Alloc *allocator, size_t off, const char *message) noexcept {
	void *buf = allocator->alloc(sizeof(SyntaxError));

	if (!buf)
		return nullptr;

	new (buf) SyntaxError(allocator, off, message);

	return (SyntaxError *)buf;
}
