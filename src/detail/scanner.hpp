#pragma once
#include <detail/token.hpp>

namespace dj::detail {
struct Scanner {
	std::string_view text{};
	Loc current{};

	static constexpr char null_v = '\0';
	static constexpr bool in_range(char const ch, char const a, char const b) { return a <= ch && ch <= b; }
	static constexpr bool is_digit(char const ch) { return in_range(ch, '0', '9'); }
	static constexpr bool is_whitespace(char const ch) { return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r'; }

	constexpr Token scan() {
		while (!at_end()) {
			char const c = advance();
			if (is_whitespace(c)) {
				++current.span.first;
				if (c == '\n') {
					++current.line;
					current.line_start = current.span.first;
				}
				continue;
			}
			auto ret = Token{};
			if (try_single(ret, c)) { return ret; }
			if (try_keyword(ret)) { return ret; }
			if (try_number(ret, c)) { return ret; }
			if (try_string(ret)) { return ret; }
		}
		return {};
	}

	constexpr bool try_single(Token& out, char const c) {
		for (Token::Type type = Token::single_range_v[0]; type < Token::single_range_v[1]; type = Token::increment(type)) {
			if (c == Token::type_string(type)[0]) {
				out = make_token(type);
				return true;
			}
		}
		return false;
	}

	constexpr bool try_keyword(Token& out) {
		for (Token::Type type = Token::keyword_range_v[0]; type < Token::keyword_range_v[1]; type = Token::increment(type)) {
			auto const str = Token::type_string(type);
			if (munch(str)) {
				current.span.last = current.span.first + str.size();
				out = make_token(type);
				return true;
			}
		}
		return false;
	}

	constexpr bool try_number(Token& out, char c) {
		if (c == '-') {
			if (!is_digit(peek())) { return false; }
			c = advance();
		}
		if (!is_digit(c)) { return false; }
		while (!at_end() && is_digit(peek())) { advance(); }
		if (match('.')) {
			while (!at_end() && is_digit(peek())) { advance(); }
		}
		if (match('e') || match('E')) {
			if (peek() == '+' || peek() == '-') { advance(); }
			while (!at_end() && is_digit(peek())) { advance(); }
		}
		out = make_token(Token::Type::eNumber);
		return true;
	}

	constexpr bool try_string(Token& out) {
		if (text[current.span.first] != '\"') { return false; }
		current.span.first = current.span.last;
		while (!at_end()) {
			if (peek() == '\"' && prev() != '\\') { break; }
			advance();
		}
		out = make_token(Token::Type::eString);
		advance();
		current.span.first = current.span.last;
		return true;
	}

	constexpr bool munch(std::string_view const str) const {
		if (current.span.first + str.size() >= text.size()) { return false; }
		return text.substr(current.span.first, str.size()) == str;
	}

	constexpr Token make_token(Token::Type type) {
		auto ret = Token{text, current, type};
		current.span.first = current.span.last;
		return ret;
	}

	constexpr bool match(char const c) {
		if (peek() == c) {
			advance();
			return true;
		}
		return false;
	}

	constexpr char advance() { return text[current.span.last++]; }
	constexpr char prev() const { return current.span.last == 0 ? '\0' : text[current.span.last - 1]; }
	constexpr char peek() const { return at_end() ? null_v : text[current.span.last]; }
	constexpr bool at_end() const { return current.span.last >= text.size(); }
};
} // namespace dj::detail
