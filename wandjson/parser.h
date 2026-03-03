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
			peff::Option<char> prevPeekedChar;
			peff::DynArray<char> intermediateBuffer;
			size_t i = 0;

			WANDJSON_FORCEINLINE ParseContext(peff::Alloc *allocator, Reader *reader) : allocator(allocator), parseFrames(allocator), reader(reader), intermediateBuffer(allocator) {}

			WANDJSON_FORCEINLINE size_t read(char *buffer, size_t size) {
				size_t szRead = 0;
				if (intermediateBuffer.size()) {
					assert(!prevPeekedChar.hasValue());
					if (intermediateBuffer.size() > size) {
						memcpy(buffer, intermediateBuffer.data(), size);
						memcpy(intermediateBuffer.data(), intermediateBuffer.data() + size, intermediateBuffer.size() - size);
						if (!intermediateBuffer.resize(size)) {
							// NOTE: Resizing the buffer without adjusting the capacity should always be true.
							std::terminate();
						}
						return size;
					} else {
						memcpy(buffer, intermediateBuffer.data(), intermediateBuffer.size());
						intermediateBuffer.clear();
						size_t szNewRead = reader->read(buffer + szRead, size - szRead);
						i += szNewRead;
						szRead += szNewRead;
						return szRead;
					}
				} else if (prevPeekedChar.hasValue()) {
					buffer[0] = prevPeekedChar.move();
					buffer += 1;
					size -= 1;
					szRead += 1;
					i += 1;
				}
				szRead += reader->read(buffer, size);
				i += szRead;
				return szRead;
			}

			WANDJSON_FORCEINLINE char nextChar() {
				if (prevPeekedChar) {
					++i;
					return prevPeekedChar.move();
				}
				char c;
				if (intermediateBuffer.size()) {
					c = intermediateBuffer.at(0);
					intermediateBuffer.popFront();
					++i;
					return c;
				}
				if (!read(&c, 1))
					return '\0';
				return c;
			}

			WANDJSON_FORCEINLINE char peekChar() {
				if (prevPeekedChar.hasValue())
					return prevPeekedChar.value();
				char c;
				if (intermediateBuffer.size()) {
					return intermediateBuffer.at(0);
				}
				if (!reader->read(&c, 1))
					return '\0';
				prevPeekedChar = +c;
				return c;
			}
		};

		WANDJSON_API bool isSpaceChar(char c);
		[[nodiscard]] WANDJSON_API char skipWhitespaces(ParseContext &parseContext);
		WANDJSON_API InternalExceptionPointer parseStringEscape(ParseContext &parseContext, peff::String &stringOut);
		WANDJSON_API InternalExceptionPointer parseString(ParseContext &parseContext, peff::String &stringOut);
		WANDJSON_API InternalExceptionPointer parseValue(Reader *reader, peff::Alloc *allocator, Value *&valueOut);
	}
}

#endif
