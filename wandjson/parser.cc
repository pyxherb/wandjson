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
			default:
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
						return withOutOfMemoryErrorIfAllocFailed(SyntaxError::alloc(parseContext.allocator, parseContext.i, "Invalid string escape"));
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
			return withOutOfMemoryErrorIfAllocFailed(SyntaxError::alloc(parseContext.allocator, parseContext.i, "Invalid string escape"));
	}

	return {};
}

WANDJSON_API InternalExceptionPointer parser::parseString(ParseContext &parseContext, peff::String &stringOut) {
	char c;
	peff::String s(parseContext.allocator);
	for (;;) {
		switch ((c = parseContext.nextChar())) {
			case '\"':
				if (auto e = parseStringEscape(parseContext, s)) {
					return e;
				}
				goto end;
			case '\\':
				break;
			case '\r':
			case '\n':
			case '\0':
				return withOutOfMemoryErrorIfAllocFailed(SyntaxError::alloc(parseContext.allocator, parseContext.i, "Unterminated string"));
			default:
				if (!s.pushBack(+c))
					return OutOfMemoryError::alloc();
		}
	}

end:
	if (parseContext.nextChar() != '\"') {
		return withOutOfMemoryErrorIfAllocFailed(SyntaxError::alloc(parseContext.allocator, parseContext.i, "Expecting \""));
	}

	stringOut = std::move(s);
	return {};
}

WANDJSON_API InternalExceptionPointer parser::parseObject(ParseContext &parseContext) {
}

WANDJSON_API InternalExceptionPointer parser::parseValue(ParseContext &parseContext, std::unique_ptr<Value, ValueDeleter> &valueOut) {
	switch (parseContext.nextChar()) {
		case '{': {
			break;
		}
		case '[': {
			break;
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
			break;
		}
		case 't': {
			if (parseContext.nextChar() != 'r') {
				return withOutOfMemoryErrorIfAllocFailed(SyntaxError::alloc(parseContext.allocator, parseContext.i, "Unrecognized character"));
			}
			if (parseContext.nextChar() != 'u') {
				return withOutOfMemoryErrorIfAllocFailed(SyntaxError::alloc(parseContext.allocator, parseContext.i, "Unrecognized character"));
			}
			if (parseContext.nextChar() != 'e') {
				return withOutOfMemoryErrorIfAllocFailed(SyntaxError::alloc(parseContext.allocator, parseContext.i, "Unrecognized character"));
			}
		}
		case 'f': {
			if (parseContext.nextChar() != 'a') {
				return withOutOfMemoryErrorIfAllocFailed(SyntaxError::alloc(parseContext.allocator, parseContext.i, "Unrecognized character"));
			}
			if (parseContext.nextChar() != 'l') {
				return withOutOfMemoryErrorIfAllocFailed(SyntaxError::alloc(parseContext.allocator, parseContext.i, "Unrecognized character"));
			}
			if (parseContext.nextChar() != 's') {
				return withOutOfMemoryErrorIfAllocFailed(SyntaxError::alloc(parseContext.allocator, parseContext.i, "Unrecognized character"));
			}
			if (parseContext.nextChar() != 'e') {
				return withOutOfMemoryErrorIfAllocFailed(SyntaxError::alloc(parseContext.allocator, parseContext.i, "Unrecognized character"));
			}
		}
		case 'n': {
			if (parseContext.nextChar() != 'u') {
				return withOutOfMemoryErrorIfAllocFailed(SyntaxError::alloc(parseContext.allocator, parseContext.i, "Unrecognized character"));
			}
			if (parseContext.nextChar() != 'l') {
				return withOutOfMemoryErrorIfAllocFailed(SyntaxError::alloc(parseContext.allocator, parseContext.i, "Unrecognized character"));
			}
			if (parseContext.nextChar() != 'l') {
				return withOutOfMemoryErrorIfAllocFailed(SyntaxError::alloc(parseContext.allocator, parseContext.i, "Unrecognized character"));
			}
		}
		default:
			return withOutOfMemoryErrorIfAllocFailed(SyntaxError::alloc(parseContext.allocator, parseContext.i, "Unrecognized character"));
	}

	return {};
}
