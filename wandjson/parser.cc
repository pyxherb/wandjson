#include "parser.h"

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

WANDJSON_API bool parser::isSpaceChar(char c) {
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

WANDJSON_API char parser::skipWhitespaces(ParseContext &parseContext) {
	char c;
	while (isSpaceChar((c = parseContext.nextChar())))
		;
	return c;
}

WANDJSON_API InternalExceptionPointer parser::parseStringEscape(ParseContext &parseContext, peff::String &stringOut) {
	switch ((parseContext.nextChar())) {
		case '\"':
			if (!stringOut.pushBack('"'))
				return OutOfMemoryError::alloc();
			break;
		case '\\':
			if (!stringOut.pushBack('\\'))
				return OutOfMemoryError::alloc();
			break;
		case '/':
			if (!stringOut.pushBack('/'))
				return OutOfMemoryError::alloc();
			break;
		case 'b':
			if (!stringOut.pushBack('\b'))
				return OutOfMemoryError::alloc();
			break;
		case 'f':
			if (!stringOut.pushBack('\f'))
				return OutOfMemoryError::alloc();
			break;
		case 'n':
			if (!stringOut.pushBack('\n'))
				return OutOfMemoryError::alloc();
			break;
		case 'r':
			if (!stringOut.pushBack('\r'))
				return OutOfMemoryError::alloc();
			break;
		case 't':
			if (!stringOut.pushBack('\t'))
				return OutOfMemoryError::alloc();
			break;
		case 'u': {
			uint16_t uc = 0;
			char d;

			for (uint8_t i = 0; i < 4; ++i) {
				switch ((d = parseContext.nextChar())) {
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
						return withOutOfMemoryErrorIfAllocFailed(SyntaxError::alloc(parseContext.allocator.get(), parseContext.i, "Invalid string escape"));
				}
			}

			if (uc <= 0x7f) {
				if (!stringOut.pushBack((char)uc))
					return OutOfMemoryError::alloc();
			} else if (uc <= 0x7ff) {
				if (!stringOut.pushBack(0b11000000 | (uc >> 6)))
					return OutOfMemoryError::alloc();
				if (!stringOut.pushBack(0b10000000 | (uc & 0b111111)))
					return OutOfMemoryError::alloc();
			} else {
				if (!stringOut.pushBack(0b111000000 | (uc >> 12)))
					return OutOfMemoryError::alloc();
				if (!stringOut.pushBack(0b10000000 | ((uc >> 6) & 0b111111)))
					return OutOfMemoryError::alloc();
				if (!stringOut.pushBack(0b10000000 | (uc & 0b111111)))
					return OutOfMemoryError::alloc();
			}

			break;
		}
		default:
			return withOutOfMemoryErrorIfAllocFailed(SyntaxError::alloc(parseContext.allocator.get(), parseContext.i, "Invalid string escape"));
	}

	return {};
}

WANDJSON_API InternalExceptionPointer parser::parseString(ParseContext &parseContext, peff::String &stringOut) {
	char c;
	peff::String s(parseContext.allocator.get());
	for (;;) {
		switch ((c = parseContext.nextChar())) {
			case '\"':
				goto end;
			case '\\':
				if (auto e = parseStringEscape(parseContext, s)) {
					return e;
				}
				break;
			case '\r':
			case '\n':
			case '\0':
				return withOutOfMemoryErrorIfAllocFailed(SyntaxError::alloc(parseContext.allocator.get(), parseContext.i, "Unterminated string"));
			default:
				if (!s.pushBack(+c))
					return OutOfMemoryError::alloc();
		}
	}

end:
	stringOut = std::move(s);
	return {};
}

WANDJSON_API InternalExceptionPointer parser::parseValue(Reader *reader, peff::Alloc *allocator, std::unique_ptr<Value, ValueDeleter> &valueOut) {
	InternalExceptionPointer e;

	ParseContext parseContext(allocator, reader);

	{
		ParseFrame parseFrame;

		if (!(parseContext.parseFrames.pushBack(std::move(parseFrame)))) {
			return OutOfMemoryError::alloc();
		}
	}

	{
		ParseFrame initialFrame;

		if (!(parseContext.parseFrames.pushBack(std::move(initialFrame)))) {
			return OutOfMemoryError::alloc();
		}
	}

	char c;

	while (parseContext.parseFrames.size() > 1) {
		c = skipWhitespaces(parseContext);
	reparseWithInitialChar:

		switch (parseContext.parseFrames.back().parseState) {
			case ParseState::Initial: {
				ParseFrame &currentFrame = parseContext.parseFrames.back();
				switch (c) {
					case '{': {
						currentFrame.parseState = ParseState::StartParsingObject;
						if (!(currentFrame.prevObject = std::unique_ptr<ObjectValue, ValueDeleter>(ObjectValue::alloc(parseContext.allocator.get())))) {
							return OutOfMemoryError::alloc();
						}

						continue;
					}
					case '[': {
						currentFrame.parseState = ParseState::StartParsingArray;
						if (!(currentFrame.prevArray = std::unique_ptr<ArrayValue, ValueDeleter>(ArrayValue::alloc(parseContext.allocator.get())))) {
							return OutOfMemoryError::alloc();
						}

						continue;
					}
					case '"': {
						peff::String s(parseContext.allocator.get());
						if ((e = parseString(parseContext, s))) {
							return e;
						}

						parseContext.parseFrames.popBack();
						if (!(parseContext.parseFrames.back().receivedValue = std::unique_ptr<Value, ValueDeleter>(StringValue::alloc(parseContext.allocator.get(), std::move(s))))) {
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
						bool isDecimal = false;
						size_t initialI = parseContext.i;

						if (!s.pushBack(+c)) {
							return OutOfMemoryError::alloc();
						}

						for (;;) {
							switch ((c = parseContext.nextChar())) {
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
									if (!s.pushBack(+c)) {
										return OutOfMemoryError::alloc();
									}
									break;
								case '.':
									goto parseNumberDigitsEnd;
								default:
									goto parseNumberEnd;
							}
						}

					parseNumberDigitsEnd:
						if (c == '.') {
							parseContext.nextChar();
							isDecimal = true;
						}

						if (isDecimal) {
							for (;;) {
								switch ((c = parseContext.nextChar())) {
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
										if (!s.pushBack(+c)) {
											return OutOfMemoryError::alloc();
										}
										break;
									case 'e':
									case 'E':
										goto parseNumberDecimalDigitsEnd;
									default:
										goto parseNumberEnd;
								}
							}

						parseNumberDecimalDigitsEnd:;
							switch ((c = parseContext.nextChar())) {
								case '+':
								case '-':
									if (!s.pushBack(+c)) {
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
									if (!s.pushBack(+c)) {
										return OutOfMemoryError::alloc();
									}
									break;
								default:
									return withOutOfMemoryErrorIfAllocFailed(SyntaxError::alloc(parseContext.allocator.get(), parseContext.i, "Malformed number"));
							}

							for (;;) {
								switch ((c = parseContext.nextChar())) {
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
										if (!s.pushBack(+c)) {
											return OutOfMemoryError::alloc();
										}
										break;
									default:
										goto parseNumberEnd;
								}
							}
						}

					parseNumberEnd:
						parseContext.parseFrames.popBack();
						if (isDecimal) {
							if (!(parseContext.parseFrames.back().receivedValue = std::unique_ptr<Value, ValueDeleter>(NumberValue::alloc(parseContext.allocator.get(), strtod(s.data(), nullptr))))) {
								return OutOfMemoryError::alloc();
							}
						} else {
							if (!(parseContext.parseFrames.back().receivedValue = std::unique_ptr<Value, ValueDeleter>(NumberValue::alloc(parseContext.allocator.get(), strtoull(s.data(), nullptr, 10))))) {
								return OutOfMemoryError::alloc();
							}
						}

						if (isSpaceChar(c)) {
							c = skipWhitespaces(parseContext);
						}

						goto reparseWithInitialChar;
					}
					case 't': {
						if (parseContext.nextChar() != 'r') {
							return withOutOfMemoryErrorIfAllocFailed(SyntaxError::alloc(parseContext.allocator.get(), parseContext.i, "Unrecognized character"));
						}
						if (parseContext.nextChar() != 'u') {
							return withOutOfMemoryErrorIfAllocFailed(SyntaxError::alloc(parseContext.allocator.get(), parseContext.i, "Unrecognized character"));
						}
						if (parseContext.nextChar() != 'e') {
							return withOutOfMemoryErrorIfAllocFailed(SyntaxError::alloc(parseContext.allocator.get(), parseContext.i, "Unrecognized character"));
						}

						parseContext.parseFrames.popBack();
						if (!(parseContext.parseFrames.back().receivedValue = std::unique_ptr<Value, ValueDeleter>(BooleanValue::alloc(parseContext.allocator.get(), true)))) {
							return OutOfMemoryError::alloc();
						}
						continue;
					}
					case 'f': {
						if (parseContext.nextChar() != 'a') {
							return withOutOfMemoryErrorIfAllocFailed(SyntaxError::alloc(parseContext.allocator.get(), parseContext.i, "Unrecognized character"));
						}
						if (parseContext.nextChar() != 'l') {
							return withOutOfMemoryErrorIfAllocFailed(SyntaxError::alloc(parseContext.allocator.get(), parseContext.i, "Unrecognized character"));
						}
						if (parseContext.nextChar() != 's') {
							return withOutOfMemoryErrorIfAllocFailed(SyntaxError::alloc(parseContext.allocator.get(), parseContext.i, "Unrecognized character"));
						}
						if (parseContext.nextChar() != 'e') {
							return withOutOfMemoryErrorIfAllocFailed(SyntaxError::alloc(parseContext.allocator.get(), parseContext.i, "Unrecognized character"));
						}

						parseContext.parseFrames.popBack();
						if (!(parseContext.parseFrames.back().receivedValue = std::unique_ptr<Value, ValueDeleter>(BooleanValue::alloc(parseContext.allocator.get(), false)))) {
							return OutOfMemoryError::alloc();
						}
						continue;
					}
					case 'n': {
						if (parseContext.nextChar() != 'u') {
							return withOutOfMemoryErrorIfAllocFailed(SyntaxError::alloc(parseContext.allocator.get(), parseContext.i, "Unrecognized character"));
						}
						if (parseContext.nextChar() != 'l') {
							return withOutOfMemoryErrorIfAllocFailed(SyntaxError::alloc(parseContext.allocator.get(), parseContext.i, "Unrecognized character"));
						}
						if (parseContext.nextChar() != 'l') {
							return withOutOfMemoryErrorIfAllocFailed(SyntaxError::alloc(parseContext.allocator.get(), parseContext.i, "Unrecognized character"));
						}

						parseContext.parseFrames.popBack();
						if (!(parseContext.parseFrames.back().receivedValue = std::unique_ptr<Value, ValueDeleter>(NullValue::alloc(parseContext.allocator.get())))) {
							return OutOfMemoryError::alloc();
						}
						continue;
					}
					default:
						return withOutOfMemoryErrorIfAllocFailed(SyntaxError::alloc(parseContext.allocator.get(), parseContext.i, "Unrecognized character"));
				}
				std::terminate();
			}
			case ParseState::ParsingObject: {
				ParseFrame &currentFrame = parseContext.parseFrames.back();

				if (!currentFrame.prevObject->data.insert(currentFrame.prevKey.release(), std::move(currentFrame.receivedValue))) {
					return OutOfMemoryError::alloc();
				}

				switch (c) {
					case ',':
						c = skipWhitespaces(parseContext);
						break;
					case '}': {
						std::unique_ptr<ObjectValue, ValueDeleter> object = std::move(currentFrame.prevObject);
						parseContext.parseFrames.popBack();
						parseContext.parseFrames.back().receivedValue = std::unique_ptr<Value, ValueDeleter>(object.release());
						continue;
					}
					default:
						return withOutOfMemoryErrorIfAllocFailed(SyntaxError::alloc(parseContext.allocator.get(), parseContext.i, "Unrecognized character"));
				}

				[[fallthrough]];
			}
			case ParseState::StartParsingObject: {
				ParseFrame &currentFrame = parseContext.parseFrames.back();

				peff::String key(parseContext.allocator.get());

				if (c != '"') {
					return withOutOfMemoryErrorIfAllocFailed(SyntaxError::alloc(parseContext.allocator.get(), parseContext.i, "Expecting \""));
				}

				if ((e = parseString(parseContext, key))) {
					return e;
				}

				c = skipWhitespaces(parseContext);

				if (c != ':') {
					return withOutOfMemoryErrorIfAllocFailed(SyntaxError::alloc(parseContext.allocator.get(), parseContext.i, "Expecting :"));
				}

				c = skipWhitespaces(parseContext);

				currentFrame.prevKey.moveFrom(std::move(key));
				currentFrame.parseState = ParseState::ParsingObject;

				ParseFrame newFrame;

				newFrame.parseState = ParseState::Initial;
				if (!parseContext.parseFrames.pushBack(std::move(newFrame))) {
					return OutOfMemoryError::alloc();
				}

				goto reparseWithInitialChar;
			}
			case ParseState::ParsingArray: {
				ParseFrame &currentFrame = parseContext.parseFrames.back();

				if (!currentFrame.prevArray->data.pushBack(std::move(currentFrame.receivedValue))) {
					return OutOfMemoryError::alloc();
				}

				switch (c) {
					case ',':
						c = skipWhitespaces(parseContext);
						break;
					case ']': {
						std::unique_ptr<ArrayValue, ValueDeleter> object = std::move(currentFrame.prevArray);
						parseContext.parseFrames.popBack();
						parseContext.parseFrames.back().receivedValue = std::unique_ptr<Value, ValueDeleter>(object.release());
						continue;
					}
					default:
						return withOutOfMemoryErrorIfAllocFailed(SyntaxError::alloc(parseContext.allocator.get(), parseContext.i, "Unrecognized character"));
				}

				[[fallthrough]];
			}
			case ParseState::StartParsingArray: {
				ParseFrame &currentFrame = parseContext.parseFrames.back();

				currentFrame.parseState = ParseState::ParsingArray;

				ParseFrame newFrame;

				newFrame.parseState = ParseState::Initial;
				if (!parseContext.parseFrames.pushBack(std::move(newFrame))) {
					return OutOfMemoryError::alloc();
				}

				goto reparseWithInitialChar;
			}
		}
	}

	valueOut = std::move(parseContext.parseFrames.back().receivedValue);

	return {};
}
