#pragma once
#include <cctype>
#include <string_view>
#include <vector>

namespace dj::detail {
enum class tk_type_t {
	lbrace,
	rbrace,
	lbracket,
	rbracket,
	comma,
	colon,
	quoted,
	unquoted,
	eof,

	_count
};

struct loc_t {
	std::size_t line{};
	std::size_t col_index{};

	constexpr void operator()(char const ch) {
		++col_index;
		if (ch == '\n') {
			++line;
			col_index = 0;
		}
	}
};

struct token_t {
	using type_t = tk_type_t;

	inline static constexpr char char_v[] = {
		'{', '}', '[', ']', ',', ':', '\0', '\0', '\0',
	};

	static constexpr token_t eof() { return {{}, type_t::eof}; }

	static constexpr token_t from_char(char const& ch) {
		for (std::size_t i = 0; i < static_cast<std::size_t>(type_t::quoted); ++i) {
			if (char_v[i] == ch) { return {{&ch, 1}, static_cast<type_t>(i)}; }
		}
		return eof();
	}

	static_assert(std::size(char_v) == static_cast<std::size_t>(type_t::_count));

	std::string_view text{};
	type_t type{};
};

struct scan_t {
	token_t token{};
	loc_t loc{};
};

inline constexpr char quote_v = '\"';
inline constexpr char escape_v = '\\';

struct scanner_t {
	using tt = tk_type_t;

	std::string_view text{};
	std::size_t index{};

	std::size_t line{1};
	std::size_t offset{};

	static bool is_space(char const ch) { return std::isspace(static_cast<unsigned char>(ch)); }

	constexpr loc_t make_loc() const { return {line, index - offset}; }

	std::size_t skip_ws(std::string_view text, std::size_t start) {
		auto ret = start;
		while (ret < text.size() && is_space(text[ret])) {
			if (text[ret] == '\n') {
				offset = ret + 1;
				++line;
			}
			++ret;
		}
		return ret;
	}

	std::size_t complete_quoted(std::string_view text, std::size_t const start) {
		auto escaped{false};
		auto end{start + 1};
		while (end < text.size()) {
			auto const escape = end > 0 && text[end - 1] == escape_v;
			if (escape && !escaped) {
				++end;
				escaped = true;
				continue;
			}
			if (text[end] == quote_v) { return end + 1; }
			++end;
			escaped = false;
		}
		return text.size();
	}

	std::size_t complete_unquoted(std::string_view text, std::size_t start) {
		auto end = start + 1;
		for (; end < text.size(); ++end) {
			auto const ch = text[end];
			if (auto tok = token_t::from_char(ch); tok.type != tt::eof || is_space(ch)) { break; }
		}
		return end;
	}

	scan_t scan() {
		if (index >= text.size()) { return {token_t::eof(), make_loc()}; }
		auto ret = scan_t{};
		auto start = skip_ws(text, index);
		ret = {token_t::from_char(text[start]), make_loc()};
		if (ret.token.type == tt::eof) {
			if (text[start] == quote_v) {
				auto const end = complete_quoted(text, start);
				if (end == std::string_view::npos) { return {token_t::eof(), ret.loc}; }
				ret.token.text = text.substr(start, end - start);
				ret.token.type = tt::quoted;
			} else {
				auto const end = complete_unquoted(text, start);
				ret.token.text = text.substr(start, end - start);
				ret.token.type = tt::unquoted;
			}
		}
		index = skip_ws(text, start + ret.token.text.size());
		return ret;
	}

	bool advance(scan_t& out) {
		out = scan();
		return out.token.type != tt::eof;
	}
};
} // namespace dj::detail
