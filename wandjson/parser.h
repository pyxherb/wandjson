#ifndef _WANDJSON_PARSER_H_
#define _WANDJSON_PARSER_H_

#include "value.h"
#include <optional>

namespace wandjson {
	class Reader {
	public:
		WANDJSON_API virtual ~Reader();
		virtual size_t read(char *buffer, size_t size) = 0;
	};

	class StringReader : public Reader {
	public:
		std::string_view src;
		size_t i = 0;

		WANDJSON_API StringReader(const std::string_view &src);
		WANDJSON_API virtual ~StringReader();
		WANDJSON_API virtual size_t read(char *buffer, size_t size) override;
	};

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
			Reader *reader;
			peff::List<ParseFrame> parseFrames;
			peff::RcObjectPtr<peff::Alloc> allocator;
			peff::String intermediateBuffer;
			size_t i = 0;

			WANDJSON_FORCEINLINE ParseContext(peff::Alloc *allocator, Reader *reader) : allocator(allocator), parseFrames(allocator), reader(reader), intermediateBuffer(allocator) {}

			WANDJSON_FORCEINLINE size_t read(char *buffer, size_t size) {
				return reader->read(buffer, size);
			}

			WANDJSON_FORCEINLINE char nextChar() {
				char c;
				if (intermediateBuffer.size()) {
					c = intermediateBuffer.at(0);
					intermediateBuffer.popFront();
					++i;
					return c;
				}
				if(!read(&c, 1))
					return '\0';
				++i;
				return c;
			}
		};

		WANDJSON_API bool isSpaceChar(char c);
		WANDJSON_API [[nodiscard]] char skipWhitespaces(ParseContext &parseContext);
		WANDJSON_API InternalExceptionPointer parseStringEscape(ParseContext &parseContext, peff::String &stringOut);
		WANDJSON_API InternalExceptionPointer parseString(ParseContext &parseContext, peff::String &stringOut);
		WANDJSON_API InternalExceptionPointer parseObject(ParseContext &parseContext, std::unique_ptr<ObjectValue, ValueDeleter> &valueOut);
		WANDJSON_API InternalExceptionPointer parseValue(Reader *reader, peff::Alloc *allocator, std::unique_ptr<Value, ValueDeleter> &valueOut);
	}
}

#endif
