#include "parser.h"

using namespace wandjson;
using namespace wandjson::parser;

WANDJSON_API void parser::skipWhitespaces(ParseContext &parseContext) {
	for (;;) {
		if (parseContext.i >= parseContext.length)
			break;
		switch (parseContext.src[parseContext.i]) {
			case ' ':
			case '\r':
			case '\t':
			case '\n':
			case '\v':
				++parseContext.i;
				continue;
			default:;
		}
		break;
	}
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

WANDJSON_API InternalExceptionPointer parser::parseValue(const char *src, size_t length, peff::Alloc *allocator, std::unique_ptr<Value, ValueDeleter> &valueOut) {
	InternalExceptionPointer e;

	ParseContext parseContext(allocator);

	parseContext.src = src;
	parseContext.length = length;

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

	do {
		skipWhitespaces(parseContext);

		ParseFrame &currentFrame = parseContext.parseFrames.back();

		switch (currentFrame.parseState) {
			case ParseState::Initial: {
				char c;
				switch ((c = parseContext.nextChar())) {
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
						bool isDecimal = false;
						size_t initialI = parseContext.i;

						for (;;) {
							switch (parseContext.peekChar()) {
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
									parseContext.nextChar();
									break;
								default:
									goto parseNumberDigitsEnd;
							}
						}

					parseNumberDigitsEnd:
						if (parseContext.peekChar() == '.') {
							parseContext.nextChar();
							isDecimal = true;
						}

						if (isDecimal) {
							for (;;) {
								switch (parseContext.peekChar()) {
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
										parseContext.nextChar();
										break;
									default:
										goto parseNumberDecimalDigitsEnd;
								}
							}
						parseNumberDecimalDigitsEnd:;

							switch (parseContext.peekChar()) {
								case 'e':
								case 'E':
									parseContext.nextChar();

									switch (parseContext.peekChar()) {
										case '+':
										case '-':
											parseContext.nextChar();
											break;
										default:;
									}

									for (;;) {
										switch (parseContext.peekChar()) {
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
												parseContext.nextChar();
												break;
											default:
												goto parseNumberExponentsEnd;
										}
									}

								parseNumberExponentsEnd:;
								default:;
							}
						}

						parseContext.parseFrames.popBack();
						if (isDecimal) {
							if (!(parseContext.parseFrames.back().receivedValue = std::unique_ptr<Value, ValueDeleter>(NumberValue::alloc(parseContext.allocator.get(), strtod(parseContext.src + initialI, nullptr))))) {
								return OutOfMemoryError::alloc();
							}
						} else {
							if (!(parseContext.parseFrames.back().receivedValue = std::unique_ptr<Value, ValueDeleter>(NumberValue::alloc(parseContext.allocator.get(), strtoull(parseContext.src + initialI, nullptr, 10))))) {
								return OutOfMemoryError::alloc();
							}
						}
						continue;
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
				if (!currentFrame.prevObject->data.insert(currentFrame.prevKey.release(), std::move(currentFrame.receivedValue))) {
					return OutOfMemoryError::alloc();
				}

				skipWhitespaces(parseContext);

				switch (parseContext.nextChar()) {
					case ',':
						break;
					case '}': {
						skipWhitespaces(parseContext);
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
				skipWhitespaces(parseContext);

				peff::String key(parseContext.allocator.get());

				if ((parseContext.nextChar()) != '"') {
					return withOutOfMemoryErrorIfAllocFailed(SyntaxError::alloc(parseContext.allocator.get(), parseContext.i, "Expecting \""));
				}

				if ((e = parseString(parseContext, key))) {
					return e;
				}

				skipWhitespaces(parseContext);

				if ((parseContext.nextChar()) != ':') {
					return withOutOfMemoryErrorIfAllocFailed(SyntaxError::alloc(parseContext.allocator.get(), parseContext.i, "Expecting :"));
				}

				skipWhitespaces(parseContext);

				currentFrame.prevKey.moveFrom(std::move(key));
				currentFrame.parseState = ParseState::ParsingObject;

				ParseFrame newFrame;

				newFrame.parseState = ParseState::Initial;
				if (!parseContext.parseFrames.pushBack(std::move(newFrame))) {
					return OutOfMemoryError::alloc();
				}

				continue;
			}
			case ParseState::ParsingArray: {
				if (!currentFrame.prevArray->data.pushBack(std::move(currentFrame.receivedValue))) {
					return OutOfMemoryError::alloc();
				}

				skipWhitespaces(parseContext);

				switch (parseContext.nextChar()) {
					case ',':
						skipWhitespaces(parseContext);
						break;
					case ']': {
						std::unique_ptr<ArrayValue, ValueDeleter> object = std::move(currentFrame.prevArray);
						parseContext.parseFrames.popBack();
						parseContext.parseFrames.back().receivedValue = std::unique_ptr<Value, ValueDeleter>(object.release());
						skipWhitespaces(parseContext);
						continue;
					}
					default:
						return withOutOfMemoryErrorIfAllocFailed(SyntaxError::alloc(parseContext.allocator.get(), parseContext.i, "Unrecognized character"));
				}

				[[fallthrough]];
			}
			case ParseState::StartParsingArray: {
				skipWhitespaces(parseContext);

				currentFrame.parseState = ParseState::ParsingArray;

				ParseFrame newFrame;

				newFrame.parseState = ParseState::Initial;
				if (!parseContext.parseFrames.pushBack(std::move(newFrame))) {
					return OutOfMemoryError::alloc();
				}

				continue;
			}
		}
	} while (parseContext.parseFrames.size() > 1);

	valueOut = std::move(parseContext.parseFrames.back().receivedValue);

	return {};
}
