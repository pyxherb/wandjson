#include "parser.h"
#include <cmath>

using namespace wandjson;
using namespace wandjson::parser;

WANDJSON_API Reader::~Reader() {
}

WANDJSON_API StringReader::StringReader(const std::string_view &src) : src(src) {
}

WANDJSON_API StringReader::~StringReader() {
}

WANDJSON_API size_t StringReader::read(char *buffer, size_t size) {
	if ((src.size() < size) || (src.size() - size < i)) {
		size_t len = src.size() - i;
		memcpy(buffer, src.data() + i, len);
		i = src.size();
		return len;
	}

	memcpy(buffer, src.data() + i, size);
	i += size;
	return size;
}

WANDJSON_API bool parser::is_space_char(char c) {
	switch (c) {
		case ' ':
		case '\r':
		case '\t':
		case '\n':
		case '\v':
			return true;
		default:;
	}
	return false;
}

WANDJSON_API char parser::skip_whitespaces(ParseContext &parse_context) {
	char c;
	while (is_space_char((c = parse_context.next_char())))
		;
	return c;
}

WANDJSON_API InternalExceptionPointer parser::parse_string_escape(ParseContext &parse_context, peff::String &string_out) {
	switch ((parse_context.next_char())) {
		case '\"':
			if (!string_out.push_back('"'))
				return OutOfMemoryError::alloc();
			break;
		case '\\':
			if (!string_out.push_back('\\'))
				return OutOfMemoryError::alloc();
			break;
		case '/':
			if (!string_out.push_back('/'))
				return OutOfMemoryError::alloc();
			break;
		case 'b':
			if (!string_out.push_back('\b'))
				return OutOfMemoryError::alloc();
			break;
		case 'f':
			if (!string_out.push_back('\f'))
				return OutOfMemoryError::alloc();
			break;
		case 'n':
			if (!string_out.push_back('\n'))
				return OutOfMemoryError::alloc();
			break;
		case 'r':
			if (!string_out.push_back('\r'))
				return OutOfMemoryError::alloc();
			break;
		case 't':
			if (!string_out.push_back('\t'))
				return OutOfMemoryError::alloc();
			break;
		case 'u': {
			uint16_t uc = 0;
			char d;

			for (uint8_t i = 0; i < 4; ++i) {
				switch ((d = parse_context.next_char())) {
					case '0':
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
					case '8':
					case '9':
						uc <<= 4;
						uc += d - '0';
						break;
					case 'a':
					case 'b':
					case 'c':
					case 'd':
					case 'e':
					case 'f':
						uc <<= 4;
						uc += d - 'a' + 10;
						break;
					case 'A':
					case 'B':
					case 'C':
					case 'D':
					case 'E':
					case 'F':
						uc <<= 4;
						uc += d - 'A' + 10;
						break;
					default:
						return with_oom_error_if_alloc_failed(SyntaxError::alloc(parse_context.allocator.get(), parse_context.i, "Invalid string escape"));
				}
			}

			if (uc <= 0x7f) {
				if (!string_out.push_back((char)uc))
					return OutOfMemoryError::alloc();
			} else if (uc <= 0x7ff) {
				if (!string_out.push_back(0b11000000 | (uc >> 6)))
					return OutOfMemoryError::alloc();
				if (!string_out.push_back(0b10000000 | (uc & 0b111111)))
					return OutOfMemoryError::alloc();
			} else {
				if (!string_out.push_back(0b111000000 | (uc >> 12)))
					return OutOfMemoryError::alloc();
				if (!string_out.push_back(0b10000000 | ((uc >> 6) & 0b111111)))
					return OutOfMemoryError::alloc();
				if (!string_out.push_back(0b10000000 | (uc & 0b111111)))
					return OutOfMemoryError::alloc();
			}

			break;
		}
		default:
			return with_oom_error_if_alloc_failed(SyntaxError::alloc(parse_context.allocator.get(), parse_context.i, "Invalid string escape"));
	}

	return {};
}

WANDJSON_API InternalExceptionPointer parser::parse_string(ParseContext &parse_context, peff::String &string_out) {
	char c;
	peff::String s(parse_context.allocator.get());
	for (;;) {
		switch ((c = parse_context.next_char())) {
			case '\"':
				goto end;
			case '\\':
				if (auto e = parse_string_escape(parse_context, s)) {
					return e;
				}
				break;
			case '\r':
			case '\n':
			case '\0':
				return with_oom_error_if_alloc_failed(SyntaxError::alloc(parse_context.allocator.get(), parse_context.i, "Unterminated string"));
			default:
				if (!s.push_back(+c))
					return OutOfMemoryError::alloc();
		}
	}

end:
	string_out = std::move(s);
	return {};
}

WANDJSON_API InternalExceptionPointer parser::parse_value(Reader *reader, peff::Alloc *allocator, Value *&value_out) {
	InternalExceptionPointer e;

	ParseContext parse_context(allocator, reader);

	{
		ParseFrame parse_frame;

		if (!(parse_context.parse_frames.push_back(std::move(parse_frame)))) {
			return OutOfMemoryError::alloc();
		}
	}

	{
		ParseFrame initial_frame;

		if (!(parse_context.parse_frames.push_back(std::move(initial_frame)))) {
			return OutOfMemoryError::alloc();
		}
	}

	char c;

	while (parse_context.parse_frames.size() > 1) {
		c = skip_whitespaces(parse_context);
	reparse_with_initial_char:

		switch (parse_context.parse_frames.back().parse_state) {
			case ParseState::Initial: {
				ParseFrame &current_frame = parse_context.parse_frames.back();
				switch (c) {
					case '{': {
						current_frame.parse_state = ParseState::StartParsingObject;
						if (!(current_frame.prev_object = std::unique_ptr<ObjectValue, ValueDeleter>(ObjectValue::alloc(parse_context.allocator.get())))) {
							return OutOfMemoryError::alloc();
						}

						continue;
					}
					case '[': {
						current_frame.parse_state = ParseState::StartParsingArray;
						if (!(current_frame.prev_array = std::unique_ptr<ArrayValue, ValueDeleter>(ArrayValue::alloc(parse_context.allocator.get())))) {
							return OutOfMemoryError::alloc();
						}

						continue;
					}
					case '"': {
						peff::String s(parse_context.allocator.get());
						if ((e = parse_string(parse_context, s))) {
							return e;
						}

						parse_context.parse_frames.pop_back();
						if (!(parse_context.parse_frames.back().received_value = std::unique_ptr<Value, ValueDeleter>(StringValue::alloc(parse_context.allocator.get(), std::move(s))))) {
							return OutOfMemoryError::alloc();
						}
						continue;
					}
					case '-':
					case '0':
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
					case '8':
					case '9': {
						peff::String s(allocator);
						bool is_decimal = false;
						size_t initial_i = parse_context.i;

						if (!s.push_back(+c)) {
							return OutOfMemoryError::alloc();
						}

						for (;;) {
							switch ((c = parse_context.next_char())) {
								case '0':
								case '1':
								case '2':
								case '3':
								case '4':
								case '5':
								case '6':
								case '7':
								case '8':
								case '9':
									if (!s.push_back(+c)) {
										return OutOfMemoryError::alloc();
									}
									break;
								case '.':
									goto parse_number_digits_end;
								default:
									goto parse_number_end;
							}
						}

					parse_number_digits_end:
						if (c == '.') {
							if (!s.push_back(+c)) {
								return OutOfMemoryError::alloc();
							}
							is_decimal = true;
						}

						if (is_decimal) {
							for (;;) {
								switch ((c = parse_context.next_char())) {
									case '0':
									case '1':
									case '2':
									case '3':
									case '4':
									case '5':
									case '6':
									case '7':
									case '8':
									case '9':
										if (!s.push_back(+c)) {
											return OutOfMemoryError::alloc();
										}
										break;
									case 'e':
									case 'E':
										if (!s.push_back(+c)) {
											return OutOfMemoryError::alloc();
										}
										goto parse_number_decimal_digits_end;
									default:
										goto parse_number_end;
								}
							}

						parse_number_decimal_digits_end:;
							switch ((c = parse_context.next_char())) {
								case '+':
								case '-':
									if (!s.push_back(+c)) {
										return OutOfMemoryError::alloc();
									}
									break;
								case '0':
								case '1':
								case '2':
								case '3':
								case '4':
								case '5':
								case '6':
								case '7':
								case '8':
								case '9':
									if (!s.push_back(+c)) {
										return OutOfMemoryError::alloc();
									}
									break;
								default:
									return with_oom_error_if_alloc_failed(SyntaxError::alloc(parse_context.allocator.get(), parse_context.i, "Malformed number"));
							}

							for (;;) {
								switch ((c = parse_context.next_char())) {
									case '0':
									case '1':
									case '2':
									case '3':
									case '4':
									case '5':
									case '6':
									case '7':
									case '8':
									case '9':
										if (!s.push_back(+c)) {
											return OutOfMemoryError::alloc();
										}
										break;
									default:
										goto parse_number_end;
								}
							}
						}

					parse_number_end:
						parse_context.parse_frames.pop_back();
						if (is_decimal) {
							double v = strtod(s.data(), nullptr);

							if ((v == INFINITY) || (std::isnan(v))) {
								parse_context.parse_frames.back().received_value = std::unique_ptr<Value, ValueDeleter>(nullptr);
							} else {
								if (!(parse_context.parse_frames.back().received_value = std::unique_ptr<Value, ValueDeleter>(NumberValue::alloc(parse_context.allocator.get(), v)))) {
									return OutOfMemoryError::alloc();
								}
							}
						} else {
							if (!(parse_context.parse_frames.back().received_value = std::unique_ptr<Value, ValueDeleter>(NumberValue::alloc(parse_context.allocator.get(), (int64_t)strtoll(s.data(), nullptr, 10))))) {
								return OutOfMemoryError::alloc();
							}
						}

						if (is_space_char(c)) {
							c = skip_whitespaces(parse_context);
						}

						goto reparse_with_initial_char;
					}
					case 't': {
						if (parse_context.next_char() != 'r') {
							return with_oom_error_if_alloc_failed(SyntaxError::alloc(parse_context.allocator.get(), parse_context.i, "Unrecognized character"));
						}
						if (parse_context.next_char() != 'u') {
							return with_oom_error_if_alloc_failed(SyntaxError::alloc(parse_context.allocator.get(), parse_context.i, "Unrecognized character"));
						}
						if (parse_context.next_char() != 'e') {
							return with_oom_error_if_alloc_failed(SyntaxError::alloc(parse_context.allocator.get(), parse_context.i, "Unrecognized character"));
						}

						parse_context.parse_frames.pop_back();
						if (!(parse_context.parse_frames.back().received_value = std::unique_ptr<Value, ValueDeleter>(BooleanValue::alloc(parse_context.allocator.get(), true)))) {
							return OutOfMemoryError::alloc();
						}
						continue;
					}
					case 'f': {
						if (parse_context.next_char() != 'a') {
							return with_oom_error_if_alloc_failed(SyntaxError::alloc(parse_context.allocator.get(), parse_context.i, "Unrecognized character"));
						}
						if (parse_context.next_char() != 'l') {
							return with_oom_error_if_alloc_failed(SyntaxError::alloc(parse_context.allocator.get(), parse_context.i, "Unrecognized character"));
						}
						if (parse_context.next_char() != 's') {
							return with_oom_error_if_alloc_failed(SyntaxError::alloc(parse_context.allocator.get(), parse_context.i, "Unrecognized character"));
						}
						if (parse_context.next_char() != 'e') {
							return with_oom_error_if_alloc_failed(SyntaxError::alloc(parse_context.allocator.get(), parse_context.i, "Unrecognized character"));
						}

						parse_context.parse_frames.pop_back();
						if (!(parse_context.parse_frames.back().received_value = std::unique_ptr<Value, ValueDeleter>(BooleanValue::alloc(parse_context.allocator.get(), false)))) {
							return OutOfMemoryError::alloc();
						}
						continue;
					}
					case 'n': {
						if (parse_context.next_char() != 'u') {
							return with_oom_error_if_alloc_failed(SyntaxError::alloc(parse_context.allocator.get(), parse_context.i, "Unrecognized character"));
						}
						if (parse_context.next_char() != 'l') {
							return with_oom_error_if_alloc_failed(SyntaxError::alloc(parse_context.allocator.get(), parse_context.i, "Unrecognized character"));
						}
						if (parse_context.next_char() != 'l') {
							return with_oom_error_if_alloc_failed(SyntaxError::alloc(parse_context.allocator.get(), parse_context.i, "Unrecognized character"));
						}

						parse_context.parse_frames.pop_back();
						parse_context.parse_frames.back().received_value = std::unique_ptr<Value, ValueDeleter>(nullptr);
						continue;
					}
					default:
						return with_oom_error_if_alloc_failed(SyntaxError::alloc(parse_context.allocator.get(), parse_context.i, "Unrecognized character"));
				}
				std::terminate();
			}
			case ParseState::ParsingObject: {
				ParseFrame &current_frame = parse_context.parse_frames.back();

				if (!current_frame.prev_object->insert(current_frame.prev_key.move(), current_frame.received_value.release())) {
					return OutOfMemoryError::alloc();
				}

				switch (c) {
					case ',':
						c = skip_whitespaces(parse_context);
						break;
					case '}': {
						std::unique_ptr<ObjectValue, ValueDeleter> object = std::move(current_frame.prev_object);
						parse_context.parse_frames.pop_back();
						parse_context.parse_frames.back().received_value = std::unique_ptr<Value, ValueDeleter>(object.release());
						continue;
					}
					default:
						return with_oom_error_if_alloc_failed(SyntaxError::alloc(parse_context.allocator.get(), parse_context.i, "Unrecognized character"));
				}

				[[fallthrough]];
			}
			case ParseState::StartParsingObject: {
				ParseFrame &current_frame = parse_context.parse_frames.back();

				peff::String key(parse_context.allocator.get());

				if (c != '"') {
					return with_oom_error_if_alloc_failed(SyntaxError::alloc(parse_context.allocator.get(), parse_context.i, "Expecting \""));
				}

				if ((e = parse_string(parse_context, key))) {
					return e;
				}

				c = skip_whitespaces(parse_context);

				if (c != ':') {
					return with_oom_error_if_alloc_failed(SyntaxError::alloc(parse_context.allocator.get(), parse_context.i, "Expecting :"));
				}

				c = skip_whitespaces(parse_context);

				current_frame.prev_key = std::move(key);
				current_frame.parse_state = ParseState::ParsingObject;

				ParseFrame new_frame;

				new_frame.parse_state = ParseState::Initial;
				if (!parse_context.parse_frames.push_back(std::move(new_frame))) {
					return OutOfMemoryError::alloc();
				}

				goto reparse_with_initial_char;
			}
			case ParseState::ParsingArray: {
				ParseFrame &current_frame = parse_context.parse_frames.back();

				if (!current_frame.prev_array->data().push_back(current_frame.received_value.release())) {
					return OutOfMemoryError::alloc();
				}

				switch (c) {
					case ',':
						c = skip_whitespaces(parse_context);
						break;
					case ']': {
						std::unique_ptr<ArrayValue, ValueDeleter> object = std::move(current_frame.prev_array);
						parse_context.parse_frames.pop_back();
						parse_context.parse_frames.back().received_value = std::unique_ptr<Value, ValueDeleter>(object.release());
						continue;
					}
					default:
						return with_oom_error_if_alloc_failed(SyntaxError::alloc(parse_context.allocator.get(), parse_context.i, "Unrecognized character"));
				}

				[[fallthrough]];
			}
			case ParseState::StartParsingArray: {
				ParseFrame &current_frame = parse_context.parse_frames.back();

				current_frame.parse_state = ParseState::ParsingArray;

				ParseFrame new_frame;

				new_frame.parse_state = ParseState::Initial;
				if (!parse_context.parse_frames.push_back(std::move(new_frame))) {
					return OutOfMemoryError::alloc();
				}

				goto reparse_with_initial_char;
			}
		}
	}

	value_out = parse_context.parse_frames.back().received_value.release();

	return {};
}
