#include "dump.h"
#include <cmath>

using namespace wandjson;

#define WANDJSON_RETURN_IF_FALSE(e) \
	if (!(e))                       \
		return false;               \
	else

WANDJSON_API Writer::~Writer() {
}

WANDJSON_API bool wandjson::_dumpString(DumpContext &dump_context, std::string_view s) {
	size_t i = 0, prev_index = 0;

	WANDJSON_RETURN_IF_FALSE(dump_context.writer->write("\"", sizeof("\"") - 1));

	for (char c; i < s.size();) {
		switch ((c = s.data()[i])) {
			case '\"': {
				WANDJSON_RETURN_IF_FALSE(dump_context.writer->write(s.data() + prev_index, i - prev_index));
				WANDJSON_RETURN_IF_FALSE(dump_context.writer->write("\\\"", sizeof("\\\"") - 1));
				prev_index = i + 1;
				break;
			}
			case '\r': {
				WANDJSON_RETURN_IF_FALSE(dump_context.writer->write(s.data() + prev_index, i - prev_index));
				WANDJSON_RETURN_IF_FALSE(dump_context.writer->write("\\r", sizeof("\\\r") - 1));
				prev_index = i + 1;
				break;
			}
			case '\n': {
				WANDJSON_RETURN_IF_FALSE(dump_context.writer->write(s.data() + prev_index, i - prev_index));
				WANDJSON_RETURN_IF_FALSE(dump_context.writer->write("\\n", sizeof("\\n") - 1));
				prev_index = i + 1;
				break;
			}
			default:;
		}
		++i;
	}

	if (i > prev_index) {
		WANDJSON_RETURN_IF_FALSE(dump_context.writer->write(s.data() + prev_index, i + 1 - prev_index));
	}

	WANDJSON_RETURN_IF_FALSE(dump_context.writer->write("\"", sizeof("\"") - 1));

	return true;
}

WANDJSON_API bool wandjson::_dumpValue(DumpContext &dump_context) {
	while (dump_context.frames.size()) {
		DumpFrame &cur_frame = dump_context.frames.back();

		switch (cur_frame.state) {
			case DumpState::None: {
				if (!cur_frame.value)
					WANDJSON_RETURN_IF_FALSE(dump_context.writer->write("null", sizeof("null") - 1));
				else
					switch (cur_frame.value->get_value_type()) {
						case ValueType::Number: {
							NumberValue *v = (NumberValue *)cur_frame.value;

							switch (v->get_number_type()) {
								case NumberKind::Integer: {
									char s[sizeof("-9223372036854775808")];

									snprintf(s, sizeof(s), "%lld", v->as_integer());

									WANDJSON_RETURN_IF_FALSE(dump_context.writer->write(s, strlen(s)));
									break;
								}
								case NumberKind::Decimal: {
									if (v->as_decimal() == INFINITY || std::isnan(v->as_decimal())) {
										WANDJSON_RETURN_IF_FALSE(dump_context.writer->write("null", sizeof("null") - 1));
									} else {
										char s[64];

										snprintf(s, sizeof(s), "%.17g", v->as_decimal());

										WANDJSON_RETURN_IF_FALSE(dump_context.writer->write(s, strlen(s)));
									}
									break;
								}
								default:
									std::terminate();
							}
							break;
						}
						case ValueType::Array: {
							BooleanValue *v = (BooleanValue *)cur_frame.value;

							cur_frame.state = DumpState::DumpingArray;

							DumpingArrayDumpFrameData data = {
								0
							};

							cur_frame.data = std::move(data);

							WANDJSON_RETURN_IF_FALSE(dump_context.writer->write("[", 1));

							continue;
						}
						case ValueType::Object: {
							ObjectValue *v = (ObjectValue *)cur_frame.value;

							cur_frame.state = DumpState::DumpingObject;

							DumpingObjectDumpFrameData data = {
								v->begin_const()
							};

							cur_frame.data = std::move(data);

							WANDJSON_RETURN_IF_FALSE(dump_context.writer->write("{", 1));

							continue;
						}
						case ValueType::Boolean: {
							BooleanValue *v = (BooleanValue *)cur_frame.value;
							if (v->data()) {
								WANDJSON_RETURN_IF_FALSE(dump_context.writer->write("true", sizeof("true") - 1));
							} else {
								WANDJSON_RETURN_IF_FALSE(dump_context.writer->write("false", sizeof("false") - 1));
							}
							break;
						}
						case ValueType::String: {
							const StringValue *v = (StringValue *)cur_frame.value;

							WANDJSON_RETURN_IF_FALSE(_dumpString(dump_context, v->data()));

							break;
						}
						default:
							std::terminate();
					}

				dump_context.frames.pop_back();
				break;
			}
			case DumpState::DumpingArray: {
				ArrayValue *v = (ArrayValue *)cur_frame.value;

				DumpingArrayDumpFrameData &data = std::get<DumpingArrayDumpFrameData>(cur_frame.data);

				DumpFrame frame = { DumpState::None, v->data().at(data.prev_index) };

				if (!dump_context.frames.push_back(std::move(frame))) {
					return false;
				}

				cur_frame.state = DumpState::DumpingArrayEnd;

				break;
			}
			case DumpState::DumpingArrayEnd: {
				ArrayValue *v = (ArrayValue *)cur_frame.value;

				DumpingArrayDumpFrameData &data = std::get<DumpingArrayDumpFrameData>(cur_frame.data);

				if (++data.prev_index >= v->data().size()) {
					WANDJSON_RETURN_IF_FALSE(dump_context.writer->write("]", 1));
					dump_context.frames.pop_back();
				} else {
					WANDJSON_RETURN_IF_FALSE(dump_context.writer->write(",", 1));
					cur_frame.state = DumpState::DumpingArray;
				}

				break;
			}
			case DumpState::DumpingObject: {
				ObjectValue *v = (ObjectValue *)cur_frame.value;

				DumpingObjectDumpFrameData &data = std::get<DumpingObjectDumpFrameData>(cur_frame.data);

				WANDJSON_RETURN_IF_FALSE(_dumpString(dump_context, data.prev_iter.key()));

				WANDJSON_RETURN_IF_FALSE(dump_context.writer->write(":", 1));

				DumpFrame frame = { DumpState::None, data.prev_iter.value() };

				if (!dump_context.frames.push_back(std::move(frame))) {
					return false;
				}

				cur_frame.state = DumpState::DumpingObjectEnd;

				break;
			}
			case DumpState::DumpingObjectEnd: {
				ObjectValue *v = (ObjectValue *)cur_frame.value;

				DumpingObjectDumpFrameData &data = std::get<DumpingObjectDumpFrameData>(cur_frame.data);

				if (++data.prev_iter == v->end_const()) {
					WANDJSON_RETURN_IF_FALSE(dump_context.writer->write("}", 1));
					dump_context.frames.pop_back();
				} else {
					WANDJSON_RETURN_IF_FALSE(dump_context.writer->write(",", 1));
					cur_frame.state = DumpState::DumpingObject;
				}
				break;
			}
			default:
				std::terminate();
		}
	}

	return true;
}

WANDJSON_API bool wandjson::dump_value(peff::Alloc *allocator, Writer *writer, Value *value) {
	DumpContext dump_context = { { allocator }, writer };

	DumpFrame frame = { DumpState::None, value };

	if (!dump_context.frames.push_back(std::move(frame))) {
		return false;
	}

	return _dumpValue(dump_context);
}
