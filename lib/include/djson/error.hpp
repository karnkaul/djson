#pragma once
#include <djson/src_loc.hpp>
#include <string>

namespace dj {
struct Error {
	enum class Type : std::int8_t {
		Unknown,
		UnrecognizedToken,
		MissingClosingQuote,
		InvalidNumber,
		InvalidEscape,
		UnexpectedToken,
		UnexpectedEof,
		MissingKey,
		MissingColon,
		MissingBrace,
		MissingBracket,
		IoError,
		UnsupportedFeature,
		COUNT_,
	};

	[[nodiscard]] auto serialize() const -> std::string;

	Type type{Type::Unknown};
	std::string token{};
	SrcLoc src_loc{};
};

auto to_string_view(Error::Type type) -> std::string_view;
auto to_string(Error const& error) -> std::string;
} // namespace dj
