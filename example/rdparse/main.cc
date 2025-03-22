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
	wandjson::InternalExceptionPointer e = wandjson::parser::parseValue(testXml.get(), size, peff::getDefaultAlloc(), v);

	if (e) {
		switch (e->kind) {
			case wandjson::ErrorKind::SyntaxError: {
				wandjson::SyntaxError *ep = ((wandjson::SyntaxError *)e.get());
				std::string_view sv(testXml.get(), ep->off);
				printf("Syntax error at %d, %d: %s\n", (int)std::count(sv.begin(), sv.end(), '\n') + 1, (int)(sv.size() - sv.find_last_of('\n')) + 1, ep->message);
				e.reset();
				break;
			}
		}
	}

	return 0;
}
