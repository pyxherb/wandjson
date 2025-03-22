#include "except_base.h"

using namespace wandjson;

WANDJSON_API InternalException::InternalException(peff::Alloc* allocator, ErrorKind kind) : allocator(allocator), kind(kind) {
}

WANDJSON_API InternalException::~InternalException() {
}
