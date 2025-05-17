#include <unit_test.hpp>
#include <print>
#include <vector>

namespace dj {
namespace {
struct Assert {};

struct State {
	std::vector<test::TestCase*> tests{};
	bool failure{};

	static auto self() -> State& {
		static auto ret = State{};
		return ret;
	}
};

void check_failed(std::string_view const type, std::string_view const expr, std::string_view const file, int const line) {
	std::println(stderr, "{} failed: '{}' [{}:{}]", type, expr, file, line);
	State::self().failure = true;
}
} // namespace

void test::check_expect(bool const pred, std::string_view const expr, std::string_view const file, int const line) {
	if (pred) { return; }
	check_failed("expectation", expr, file, line);
}

void test::check_assert(bool const pred, std::string_view const expr, std::string_view const file, int const line) {
	if (pred) { return; }
	check_failed("assertion", expr, file, line);
	throw Assert{};
}

auto test::run_tests() -> int {
	auto& state = State::self();
	for (auto* test : state.tests) {
		try {
			std::println("[{}]", test->name);
			test->run();
		} catch (Assert const& /*a*/) {}
	}

	if (state.failure) {
		std::println("FAILED");
		return EXIT_FAILURE;
	}

	std::println("passed");
	return EXIT_SUCCESS;
}

namespace test {
TestCase::TestCase(std::string_view const name) : name(name) { State::self().tests.push_back(this); }
} // namespace test
} // namespace dj
