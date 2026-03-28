#include <wandjson/parser.h>
#include <wandjson/dump.h>
#include <peff/advutils/unique_ptr.h>
#include <fstream>
#include <algorithm>

class ANSIWriter : public wandjson::Writer {
public:
	WANDJSON_API virtual ~ANSIWriter() {
	}

	virtual bool write(const char* src, size_t size) override {
		fwrite(src, size, 1, stdout);
		return true;
	}
};

int main() {
#ifdef _MSC_VER
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	std::ifstream is("test.json");

	is.seekg(0, std::ios::end);
	size_t size = is.tellg();
	is.seekg(0, std::ios::beg);

	std::unique_ptr<char[]> test_json(std::make_unique<char[]>(size));
	is.read(test_json.get(), size);

	peff::UniquePtr<wandjson::Value, wandjson::ValueDeleter> v;

	std::string_view sv(test_json.get(), size);
	wandjson::StringReader sr(sv);

	wandjson::InternalExceptionPointer e = wandjson::parser::parse_value(&sr, peff::default_allocator(), v.get_ref());

	if (e) {
		switch (e->kind) {
			case wandjson::ErrorKind::SyntaxError: {
				wandjson::SyntaxError *ep = ((wandjson::SyntaxError *)e.get());
				std::string_view subview(sv.substr(0, ep->off));
				printf("Syntax error at %d, %d: %s\n", (int)std::count(sv.data(), sv.data() + ep->off + 1, '\n') + 1, (int)(ep->off - subview.find_last_of('\n')) + 1, ep->message);
				e.reset();
				break;
			}
		}
	}

	puts("Dumping:");

	ANSIWriter writer;

	if (!wandjson::dump_value(peff::default_allocator(), &writer, v.get())) {
		std::terminate();
	}

	fflush(stdout);

	return 0;
}
