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
		enum class ParseState : uint8_t {
			Initial = 0,
			StartParsingObject,
			ParsingObject,
			StartParsingArray,
			ParsingArray
		};

		struct ParseFrame {
			std::unique_ptr<ObjectValue, ValueDeleter> prev_object;
			peff::Option<peff::String> prev_key;

			std::unique_ptr<ArrayValue, ValueDeleter> prev_array;

			std::unique_ptr<Value, ValueDeleter> received_value;

			ParseState parse_state = ParseState::Initial;
		};

		struct ParseContext {
			Reader *reader;
			peff::List<ParseFrame> parse_frames;
			peff::RcObjectPtr<peff::Alloc> allocator;
			peff::Option<char> prev_peeked_char;
			peff::DynArray<char> intermediate_buffer;
			size_t i = 0;

			WANDJSON_FORCEINLINE ParseContext(peff::Alloc *allocator, Reader *reader) : allocator(allocator), parse_frames(allocator), reader(reader), intermediate_buffer(allocator) {}

			WANDJSON_FORCEINLINE size_t read(char *buffer, size_t size) {
				size_t sz_read = 0;
				if (intermediate_buffer.size()) {
					assert(!prev_peeked_char.has_value());
					if (intermediate_buffer.size() > size) {
						memcpy(buffer, intermediate_buffer.data(), size);
						memcpy(intermediate_buffer.data(), intermediate_buffer.data() + size, intermediate_buffer.size() - size);
						if (!intermediate_buffer.resize(size)) {
							// NOTE: Resizing the buffer without adjusting the capacity should always be true.
							std::terminate();
						}
						return size;
					} else {
						memcpy(buffer, intermediate_buffer.data(), intermediate_buffer.size());
						intermediate_buffer.clear();
						size_t sz_new_read = reader->read(buffer + sz_read, size - sz_read);
						i += sz_new_read;
						sz_read += sz_new_read;
						return sz_read;
					}
				} else if (prev_peeked_char.has_value()) {
					buffer[0] = prev_peeked_char.move();
					buffer += 1;
					size -= 1;
					sz_read += 1;
					i += 1;
				}
				sz_read += reader->read(buffer, size);
				i += sz_read;
				return sz_read;
			}

			WANDJSON_FORCEINLINE char next_char() {
				if (prev_peeked_char) {
					++i;
					return prev_peeked_char.move();
				}
				char c;
				if (intermediate_buffer.size()) {
					c = intermediate_buffer.at(0);
					intermediate_buffer.pop_front();
					++i;
					return c;
				}
				if (!read(&c, 1))
					return '\0';
				return c;
			}

			WANDJSON_FORCEINLINE char peek_char() {
				if (prev_peeked_char.has_value())
					return prev_peeked_char.value();
				char c;
				if (intermediate_buffer.size()) {
					return intermediate_buffer.at(0);
				}
				if (!reader->read(&c, 1))
					return '\0';
				prev_peeked_char = +c;
				return c;
			}
		};

		WANDJSON_API bool is_space_char(char c);
		[[nodiscard]] WANDJSON_API char skip_whitespaces(ParseContext &parse_context);
		WANDJSON_API InternalExceptionPointer parse_string_escape(ParseContext &parse_context, peff::String &string_out);
		WANDJSON_API InternalExceptionPointer parse_string(ParseContext &parse_context, peff::String &string_out);
		WANDJSON_API InternalExceptionPointer parse_value(Reader *reader, peff::Alloc *allocator, Value *&value_out);
	}
}

#endif
