#include <test/test.hpp>
#include <iostream>

namespace dj {
namespace {
struct Error {};
bool g_error{};
} // namespace

bool test::check(bool pred, std::string_view prefix, std::string_view expr, std::int32_t line) {
	if (pred) { return true; }
	std::cerr << prefix << " failed: " << expr << " at line " << line << '\n';
	return false;
}

void test::do_expect(bool pred, std::string_view expr, std::int32_t line) {
	if (!check(pred, "expectation", expr, line)) { g_error = true; }
}

void test::do_assert(bool pred, std::string_view expr, std::int32_t line) {
	if (!check(pred, "assertion", expr, line)) {
		g_error = true;
		throw Error{};
	}
}

void test::Runner::add(std::string name, std::function<void()> test) { tests.push_back({std::move(test), std::move(name)}); }

bool test::Runner::run() const {
	if (tests.empty()) {
		std::cout << "no tests to run\n";
		return true;
	}
	for (auto const& [test, name] : tests) {
		try {
			test();
		} catch (Error const&) {}
		if (g_error) {
			std::cerr << "\ntest failed: [" << name << "]\n";
			g_error = false;
			return false;
		}
		std::cout << "pass: "
				  << "[" << name << "]\n";
	}
	std::cout << "\nok: [" << tests.size() << "] test(s) passed\n";
	return true;
}
} // namespace dj
