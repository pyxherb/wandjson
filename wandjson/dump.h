#ifndef _WANDJSON_DUMP_H_
#define _WANDJSON_DUMP_H_

#include "except.h"
#include "value.h"

namespace wandjson {
	class Writer {
	public:
		virtual void write();
	};
}

#endif
