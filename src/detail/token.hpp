#pragma once
#include <detail/loc.hpp>

namespace dj::detail {
struct Token {
	enum class Type { eEof, eBraceL, eBraceR, eSqBrL, eSqBrR, eComma, eColon, eTrue, eFalse, eNull, eNumber, eString, eCOUNT_ };
	static constexpr std::string_view type_str_v[] = {"eof", "{", "}", "[", "]", ",", ":", "true", "false", "null", "number", "string"};
	static_assert(std::size(type_str_v) == static_cast<std::size_t>(Type::eCOUNT_));
	static constexpr auto type_string(Type const type) { return type_str_v[static_cast<std::size_t>(type)]; }
	static constexpr auto increment(Type const type) { return static_cast<Type>(static_cast<std::underlying_type_t<Type>>(type) + 1); }

	static constexpr Type single_range_v[] = {Type::eBraceL, Type::eTrue};
	static constexpr Type keyword_range_v[] = {Type::eTrue, Type::eNumber};

	std::string_view full_text{};
	Loc loc{};
	Type type{};

	constexpr std::string_view lexeme(std::string_view text) const { return loc.span.view(text); }
	constexpr operator bool() const { return type != Type::eEof; }
};
} // namespace dj::detail
