#include <djson/build_version.hpp>
#include <unit_test.hpp>
#include <print>

auto main() -> int {
	try {
		std::println("- djson {} -", dj::build_version_v);
		return dj::test::run_tests();
	} catch (std::exception const& e) {
		std::println(stderr, "PANIC: {}", e.what());
		return EXIT_FAILURE;
	} catch (...) {
		std::println(stderr, "PANIC");
		return EXIT_FAILURE;
	}
}
