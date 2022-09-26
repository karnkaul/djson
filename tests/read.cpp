#include <djson/json.hpp>
#include <test/test.hpp>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace {
namespace fs = std::filesystem;

void add(dj::test::Runner& out, fs::path const& root) {
	for (auto const& file : fs::recursive_directory_iterator(root)) {
		auto const path = file.path();
		if (!fs::is_regular_file(path) || path.extension() != ".json") { continue; }
		out.add(path.filename().generic_string(), [path] {
			auto json = dj::Json::from_file(path.generic_string().c_str());
			EXPECT(!json.is_null());
			std::cout << "\n == " << path << " ==\n" << json << "\n";
		});
	}
}

fs::path find_jsons(fs::path start) {
	for (; !start.empty() && start.parent_path() != start; start = start.parent_path()) {
		auto path = start / "jsons";
		if (fs::is_directory(path)) { return path; }
		path = start / "tests/jsons";
		if (fs::is_directory(path)) { return path; }
	}
	return {};
}
} // namespace

int main(int, char** argv) {
	auto const jsons = find_jsons(fs::absolute(argv[0]));
	if (jsons.empty()) {
		std::cerr << "Could not locate jsons/\n";
		return EXIT_FAILURE;
	}
	auto runner = dj::test::Runner{};
	add(runner, jsons);
	if (!runner.run()) { return EXIT_FAILURE; }
}
