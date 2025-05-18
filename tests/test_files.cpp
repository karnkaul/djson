#include <djson/json.hpp>
#include <unit_test.hpp>
#include <filesystem>
#include <print>
#include <vector>

namespace {
namespace fs = std::filesystem;

auto get_paths() {
	auto ret = std::vector<fs::path>{};
	auto err = std::error_code{};
	for (auto const& it : fs::directory_iterator{"tests/jsons", err}) {
		if (!it.is_regular_file(err)) { continue; }
		auto const& path = it.path();
		if (path.extension().generic_string() != ".json") { continue; }
		ret.push_back(path);
	}
	return ret;
}

TEST(test_files) {
	auto const paths = get_paths();
	EXPECT(!paths.empty());

	for (auto const& path : paths) {
		std::println("-- {}", path.filename().generic_string());
		auto const result = dj::Json::from_file(path.string());
		EXPECT(result);
		if (!result) { continue; }
		EXPECT(!result->is_null());
	}
}
} // namespace
