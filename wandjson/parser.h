#ifndef _WANDJSON_PARSER_H_
#define _WANDJSON_PARSER_H_

#include "value.h"
#include <optional>

namespace wandjson {
	namespace parser {
		struct ParseContext {
			peff::Alloc *allocator;
			const char *src;
			size_t length;
			size_t i = 0;

			WANDJSON_FORCEINLINE char peekChar() {
				if (i >= length)
					return '\0';
				return src[i];
			}

			WANDJSON_FORCEINLINE char nextChar() {
				if (i >= length)
					return '\0';
				return src[i++];
			}
		};

		WANDJSON_API void skipWhitespaces(ParseContext &parseContext);
		WANDJSON_API InternalExceptionPointer parseStringEscape(ParseContext &parseContext, peff::String &stringOut);
		WANDJSON_API InternalExceptionPointer parseString(ParseContext &parseContext, peff::String &stringOut);
		WANDJSON_API InternalExceptionPointer parseObject(ParseContext &parseContext);
		WANDJSON_API InternalExceptionPointer parseValue(ParseContext &parseContext, std::unique_ptr<Value, ValueDeleter> &valueOut);
	}
}

#endif
