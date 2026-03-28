#ifndef _WANDJSON_DUMP_H_
#define _WANDJSON_DUMP_H_

#include "except.h"
#include "value.h"
#include <peff/containers/list.h>
#include <peff/containers/set.h>
#include <variant>

namespace wandjson {
	class Writer {
	public:
		WANDJSON_API virtual ~Writer();
		[[nodiscard]] virtual bool write(const char *src, size_t size) = 0;
	};

	enum class DumpState : uint8_t {
		None = 0,
		DumpingObject,
		DumpingObjectEnd,
		DumpingArray,
		DumpingArrayEnd
	};

	struct DumpingObjectDumpFrameData {
		ObjectValue::ConstIterator prev_iter;
	};

	struct DumpingArrayDumpFrameData {
		size_t prev_index;
	};

	struct DumpFrame {
		DumpState state;
		Value *value;

		std::variant<std::monostate, DumpingObjectDumpFrameData, DumpingArrayDumpFrameData> data;
	};

	struct DumpContext {
		peff::List<DumpFrame> frames;
		Writer *writer;
	};

	WANDJSON_API bool _dumpString(DumpContext &dump_context, std::string_view s);
	WANDJSON_API bool _dumpValue(DumpContext &dump_context);

	WANDJSON_API bool dump_value(peff::Alloc *allocator, Writer *writer, Value *value);
}

#endif
