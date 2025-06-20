#include <djson/json.hpp>
#include <unit_test.hpp>
#include <filesystem>
#include <print>
#include <vector>

namespace {
namespace fs = std::filesystem;

auto locate_jsons_dir() -> fs::path {
	auto err = std::error_code{};
	for (auto dir = fs::current_path(); !dir.empty() && dir.has_parent_path(); dir = dir.parent_path()) {
		auto const ret = dir / "tests/jsons";
		if (fs::is_directory(ret, err)) { return ret; }
	}
	return {};
}

auto get_paths(fs::path const& dir) -> std::vector<fs::path> {
	auto ret = std::vector<fs::path>{};
	auto err = std::error_code{};
	for (auto const& it : fs::directory_iterator{dir, err}) {
		if (!it.is_regular_file(err)) { continue; }
		auto const& path = it.path();
		if (path.extension().generic_string() != ".json") { continue; }
		ret.push_back(path);
	}
	return ret;
}

TEST(test_files) {
	auto const jsons_dir = locate_jsons_dir();
	if (jsons_dir.empty()) {
		std::println("skipping test: could not locate 'tests/jsons' directory");
		return;
	}

	auto const paths = get_paths(jsons_dir);
	if (paths.empty()) {
		std::println("skipping test: no JSON files found in '{}'", jsons_dir.generic_string());
		return;
	}

	for (auto const& path : paths) {
		std::println("-- {}", path.filename().generic_string());
		auto const result = dj::Json::from_file(path.string());
		EXPECT(result);
		if (!result) { continue; }
		EXPECT(!result->is_null());
	}
}
} // namespace
