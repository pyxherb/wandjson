#include "dump.h"

using namespace wandjson;

#define WANDJSON_RETURN_IF_FALSE(e) \
	if (!(e)) return false;

WANDJSON_API Writer::~Writer() {
}

WANDJSON_API bool wandjson::_dumpString(DumpContext &dumpContext, std::string_view s) {
	size_t i = 0, prevIndex = 0;

	WANDJSON_RETURN_IF_FALSE(dumpContext.writer->write("\"", sizeof("\"") - 1));

	for (char c; i < s.size();) {
		switch ((c = s.data()[i])) {
			case '\"': {
				WANDJSON_RETURN_IF_FALSE(dumpContext.writer->write(s.data() + prevIndex, i - prevIndex));
				WANDJSON_RETURN_IF_FALSE(dumpContext.writer->write("\\\"", sizeof("\\\"") - 1));
				prevIndex = i + 1;
				break;
			}
			case '\r': {
				WANDJSON_RETURN_IF_FALSE(dumpContext.writer->write(s.data() + prevIndex, i - prevIndex));
				WANDJSON_RETURN_IF_FALSE(dumpContext.writer->write("\\r", sizeof("\\\r") - 1));
				prevIndex = i + 1;
				break;
			}
			case '\n': {
				WANDJSON_RETURN_IF_FALSE(dumpContext.writer->write(s.data() + prevIndex, i - prevIndex));
				WANDJSON_RETURN_IF_FALSE(dumpContext.writer->write("\\n", sizeof("\\n") - 1));
				prevIndex = i + 1;
				break;
			}
			default:;
		}
		++i;
	}

	if (i > prevIndex) {
		WANDJSON_RETURN_IF_FALSE(dumpContext.writer->write(s.data() + prevIndex, i + 1 - prevIndex));
	}

	WANDJSON_RETURN_IF_FALSE(dumpContext.writer->write("\"", sizeof("\"") - 1));

	return true;
}

WANDJSON_API bool wandjson::_dumpValue(DumpContext &dumpContext) {
	while (dumpContext.frames.size()) {
		DumpFrame &curFrame = dumpContext.frames.back();

		switch (curFrame.state) {
			case DumpState::None: {
				switch (curFrame.value->getValueType()) {
					case ValueType::Number: {
						NumberValue *v = (NumberValue *)curFrame.value;

						switch (v->getNumberKind()) {
							case NumberKind::Integer: {
								char s[sizeof("-9223372036854775808")];

								snprintf(s, sizeof(s), "%lld", v->asInteger());

								WANDJSON_RETURN_IF_FALSE(dumpContext.writer->write(s, strlen(s)));
								break;
							}
							case NumberKind::Decimal: {
								if (v->asDecimal() == INFINITY || isnan(v->asDecimal())) {
									WANDJSON_RETURN_IF_FALSE(dumpContext.writer->write("null", sizeof("null") - 1));
								} else {
									char s[64];

									snprintf(s, sizeof(s), "%.17g", v->asDecimal());

									WANDJSON_RETURN_IF_FALSE(dumpContext.writer->write(s, strlen(s)));
								}
								break;
							}
							default:
								std::terminate();
						}
						break;
					}
					case ValueType::Array: {
						BooleanValue *v = (BooleanValue *)curFrame.value;

						curFrame.state = DumpState::DumpingArray;

						DumpingArrayDumpFrameData data = {
							0
						};

						curFrame.data = std::move(data);

						WANDJSON_RETURN_IF_FALSE(dumpContext.writer->write("[", 1));

						continue;
					}
					case ValueType::Object: {
						ObjectValue *v = (ObjectValue *)curFrame.value;

						curFrame.state = DumpState::DumpingObject;

						DumpingObjectDumpFrameData data = {
							v->beginConst()
						};

						curFrame.data = std::move(data);

						WANDJSON_RETURN_IF_FALSE(dumpContext.writer->write("{", 1));

						continue;
					}
					case ValueType::Boolean: {
						BooleanValue *v = (BooleanValue *)curFrame.value;
						if (v->data()) {
							WANDJSON_RETURN_IF_FALSE(dumpContext.writer->write("true", sizeof("true") - 1));
						} else {
							WANDJSON_RETURN_IF_FALSE(dumpContext.writer->write("false", sizeof("false") - 1));
						}
						break;
					}
					case ValueType::Null:
						WANDJSON_RETURN_IF_FALSE(dumpContext.writer->write("null", sizeof("null") - 1));
						break;
					case ValueType::String: {
						const StringValue *v = (StringValue *)curFrame.value;

						WANDJSON_RETURN_IF_FALSE(_dumpString(dumpContext, v->data()));

						break;
					}
					default:
						std::terminate();
				}

				dumpContext.frames.popBack();
				break;
			}
			case DumpState::DumpingArray: {
				ArrayValue *v = (ArrayValue *)curFrame.value;

				DumpingArrayDumpFrameData &data = std::get<DumpingArrayDumpFrameData>(curFrame.data);

				DumpFrame frame = { DumpState::None, v->data().at(data.prevIndex) };

				if (!dumpContext.frames.pushBack(std::move(frame))) {
					return false;
				}

				curFrame.state = DumpState::DumpingArrayEnd;

				break;
			}
			case DumpState::DumpingArrayEnd: {
				ArrayValue *v = (ArrayValue *)curFrame.value;

				DumpingArrayDumpFrameData &data = std::get<DumpingArrayDumpFrameData>(curFrame.data);

				if (++data.prevIndex >= v->data().size()) {
					WANDJSON_RETURN_IF_FALSE(dumpContext.writer->write("]", 1));
					dumpContext.frames.popBack();
				} else {
					WANDJSON_RETURN_IF_FALSE(dumpContext.writer->write(",", 1));
					curFrame.state = DumpState::DumpingArray;
				}

				break;
			}
			case DumpState::DumpingObject: {
				ObjectValue *v = (ObjectValue *)curFrame.value;

				DumpingObjectDumpFrameData &data = std::get<DumpingObjectDumpFrameData>(curFrame.data);

				WANDJSON_RETURN_IF_FALSE(_dumpString(dumpContext, data.prevIterator.key()));

				WANDJSON_RETURN_IF_FALSE(dumpContext.writer->write(":", 1));

				DumpFrame frame = { DumpState::None, data.prevIterator.value() };

				if (!dumpContext.frames.pushBack(std::move(frame))) {
					return false;
				}

				curFrame.state = DumpState::DumpingObjectEnd;

				break;
			}
			case DumpState::DumpingObjectEnd: {
				ObjectValue *v = (ObjectValue *)curFrame.value;

				DumpingObjectDumpFrameData &data = std::get<DumpingObjectDumpFrameData>(curFrame.data);

				if (++data.prevIterator == v->endConst()) {
					WANDJSON_RETURN_IF_FALSE(dumpContext.writer->write("}", 1));
					dumpContext.frames.popBack();
				} else {
					WANDJSON_RETURN_IF_FALSE(dumpContext.writer->write(",", 1));
					curFrame.state = DumpState::DumpingObject;
				}
				break;
			}
			default:
				std::terminate();
		}
	}

	return true;
}

WANDJSON_API bool wandjson::dumpValue(peff::Alloc *allocator, Writer *writer, Value *value) {
	DumpContext dumpContext = { { allocator }, writer };

	DumpFrame frame = { DumpState::None, value };

	if (!dumpContext.frames.pushBack(std::move(frame))) {
		return false;
	}

	return _dumpValue(dumpContext);
}
