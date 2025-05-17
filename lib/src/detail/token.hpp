#pragma once
#include <djson/src_loc.hpp>
#include <array>
#include <cassert>
#include <string_view>
#include <variant>

namespace dj::detail {
using namespace std::string_view_literals;

namespace token {
struct Eof {};

enum class Operator : std::int8_t { Null, True, False, Colon, Comma, BraceLeft, BraceRight, SquareLeft, SquareRight, COUNT_ };
inline constexpr auto operator_str_v = std::array{"null"sv, "true"sv, "false"sv, ":"sv, ","sv, "{"sv, "}"sv, "["sv, "]"sv};
static_assert(operator_str_v.size() == std::size_t(Operator::COUNT_));

struct Number {
	std::string_view raw_str{};
};

struct String {
	std::string_view escaped{};
};
} // namespace token

using TokenType = std::variant<token::Eof, token::Operator, token::String, token::Number>;

struct Token {
	using Type = TokenType;

	template <typename T>
	[[nodiscard]] constexpr auto is() const -> bool {
		return std::holds_alternative<T>(type);
	}

	[[nodiscard]] constexpr auto is_operator(token::Operator const op) const {
		if (auto const* o = std::get_if<token::Operator>(&type)) { return *o == op; }
		return false;
	}

	Type type{token::Eof{}};
	std::string_view lexeme{};
	SrcLoc src_loc{};
};
} // namespace dj::detail
