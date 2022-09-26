#pragma once
#include <string_view>

namespace dj {
struct ParseError {
	struct Handler;

	std::string_view message{};
	std::string_view token{};
	std::uint32_t line{};
	std::uint32_t column{};
};

struct ParseError::Handler {
	virtual void operator()(ParseError const&) = 0;
};
} // namespace dj
