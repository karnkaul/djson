#pragma once
#include <djson/src_loc.hpp>
#include <string>

namespace dj {
/// \brief Various kinds of parse and IO errors.
/// Contains contextual token and source location if parse error.
struct Error {
	enum class Type : std::int8_t {
		Unknown,
		UnrecognizedToken,
		MissingClosingQuote,
		InvalidNumber,
		InvalidEscape,
		UnexpectedToken,
		UnexpectedComment,
		UnexpectedEof,
		MissingKey,
		MissingColon,
		MissingBrace,
		MissingBracket,
		MissingEndComment,
		IoError,
		UnsupportedFeature,
		COUNT_,
	};

	Type type{Type::Unknown};
	std::string token{};
	SrcLoc src_loc{};
};

/// \brief Obtain stringified Error Type.
auto to_string_view(Error::Type type) -> std::string_view;

/// \brief Obtain print-friendly error string.
auto to_string(Error const& error) -> std::string;
} // namespace dj
