#ifndef _WANDJSON_DUMP_H_
#define _WANDJSON_DUMP_H_

#include "except.h"
#include "value.h"
#include <peff/containers/list.h>
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
		decltype(std::declval<ObjectValue>().data)::ConstIterator prevIterator;
	};

	struct DumpingArrayDumpFrameData {
		size_t prevIndex;
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

	WANDJSON_API bool _dumpString(DumpContext &dumpContext, std::string_view s);
	WANDJSON_API bool _dumpValue(DumpContext &dumpContext);

	WANDJSON_API bool dumpValue(peff::Alloc *allocator, Writer *writer, Value *value);
}

#endif
