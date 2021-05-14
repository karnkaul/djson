#pragma once
#include <array>
#include <string>

namespace dj::detail {
struct location_t {
	std::size_t line = 0;
	std::size_t col = 0;
};

template <typename Enum>
struct lexeme {
	std::string_view text;
	location_t loc;
	Enum type = {};

	std::size_t hash() const noexcept { return std::hash<std::string_view>{}(text); }
};

enum class tk_type { curly_open, curly_close, square_open, square_close, colon, comma, escape, value };
constexpr std::array tokens_arr = {'{', '}', '[', ']', ':', ',', '\\', '\0'};
enum class nd_type { scalar, array, object, unknown };

using token_t = lexeme<tk_type>;
using payload_t = lexeme<nd_type>;

constexpr char token_char(tk_type type) noexcept { return tokens_arr[(std::size_t)type]; }
} // namespace dj::detail
