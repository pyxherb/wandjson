#ifndef _WANDJSON_PARSER_H_
#define _WANDJSON_PARSER_H_

#include "value.h"
#include <optional>

namespace wandjson {
	namespace parser {
		enum class ParseState {
			Initial = 0,
			StartParsingObject,
			ParsingObject,
			StartParsingArray,
			ParsingArray
		};

		struct ParseFrame {
			ParseState parseState = ParseState::Initial;

			std::unique_ptr<ObjectValue, ValueDeleter> prevObject;
			peff::Uninitialized<peff::String> prevKey;

			std::unique_ptr<ArrayValue, ValueDeleter> prevArray;

			std::unique_ptr<Value, ValueDeleter> receivedValue;
		};

		struct ParseContext {
			peff::List<ParseFrame> parseFrames;
			peff::RcObjectPtr<peff::Alloc> allocator;
			const char *src;
			size_t length;
			size_t i = 0;

			WANDJSON_FORCEINLINE ParseContext(peff::Alloc *allocator) : allocator(allocator), parseFrames(allocator) {}

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
		WANDJSON_API InternalExceptionPointer parseObject(ParseContext &parseContext, std::unique_ptr<ObjectValue, ValueDeleter> &valueOut);
		WANDJSON_API InternalExceptionPointer parseValue(const char *src, size_t length, peff::Alloc *allocator, std::unique_ptr<Value, ValueDeleter> &valueOut);
	}
}

#endif
