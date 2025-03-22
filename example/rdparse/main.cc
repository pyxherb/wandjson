#include <wandjson/parser.h>
#include <fstream>

int main() {
	std::ifstream is("test.xml");

	is.seekg(0, std::ios::end);
	size_t size = is.tellg();
	is.seekg(0, std::ios::beg);

	std::unique_ptr<char[]> testXml(std::make_unique<char[]>(size));
	is.read(testXml.get(), size);

	return 0;
}
