#pragma once
#include <detail/lexer.hpp>
#include <dumb_json/error_handler.hpp>
#include <str_format/str_format.hpp>

namespace dj::detail {
template <typename T, typename U>
void fail_expect(T&& expected, U&& obtained, std::string_view label, location_t const& loc) {
	g_error({kt::format_str("Expected {}: `{}`, obtained: `{}`", label, expected, obtained), loc.line, loc.col});
}

template <typename T>
void fail_expect(T&& expected, token_t const& token) {
	g_error({kt::format_str("Expected token: `{}`, obtained: `{}`", expected, token.text), token.loc.line, token.loc.col});
}

template <typename T>
void fail_unexpect(T&& unexpected, std::string_view label, location_t const& loc) {
	g_error({kt::format_str("Unexpected {}: `{}`", label, unexpected), loc.line, loc.col});
}

inline void fail_unexpect(token_t const& unexpected) {
	g_error({kt::format_str("Unexpected token: `{}`", unexpected.text), unexpected.loc.line, unexpected.loc.col});
}

template <typename T>
bool test_expect(std::optional<token_t> token, lexer const& lexer, tk_type type, T&& expected) {
	if (!token) {
		detail::fail_expect(expected, "[EOF]", "token", lexer.location());
		return false;
	}
	if (token->type != type) {
		detail::fail_expect(expected, *token);
		return false;
	}
	return true;
}

inline bool test_expect(std::optional<token_t> token, lexer const& lexer, tk_type type) { return test_expect(token, lexer, type, token_char(type)); }
} // namespace dj::detail
