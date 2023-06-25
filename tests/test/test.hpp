#pragma once
#include <cstdint>
#include <functional>
#include <string>

#define ASSERT(pred) ::dj::test::do_assert(pred, #pred, __LINE__)
#define EXPECT(pred) ::dj::test::do_expect(pred, #pred, __LINE__)

namespace dj::test {

bool check(bool pred, std::string_view prefix, std::string_view expr, std::int32_t line);
void do_expect(bool pred, std::string_view expr, std::int32_t line);
void do_assert(bool pred, std::string_view expr, std::int32_t line);

struct Runner {
	struct Test {
		std::function<void()> func{};
		std::string name{};
	};

	std::vector<Test> tests{};

	void add(std::string name, std::function<void()> test);
	bool run() const;
};
} // namespace dj::test
