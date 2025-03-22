#include <wandjson/parser.h>
#include <fstream>

int main() {
	std::ifstream is("test.json");

	is.seekg(0, std::ios::end);
	size_t size = is.tellg();
	is.seekg(0, std::ios::beg);

	std::unique_ptr<char[]> testXml(std::make_unique<char[]>(size));
	is.read(testXml.get(), size);

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	std::string_view sv(testXml.get(), size);
	wandjson::StringReader sr(sv);

	wandjson::InternalExceptionPointer e = wandjson::parser::parseValue(&sr, peff::getDefaultAlloc(), v);

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

	return 0;
}
