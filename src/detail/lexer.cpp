#include <cassert>
#include <unordered_map>
#include <detail/lexer.hpp>

namespace dj::detail {
namespace {
// clang-format off
std::unordered_map<char, tk_type> const char_tokens = {
	{'{', tk_type::curly_open},
	{'}', tk_type::curly_close},
	{'[', tk_type::square_open},
	{']', tk_type::square_close},
	{':', tk_type::colon},
	{',', tk_type::comma},
	{'\\', tk_type::escape},
};
// clang-format on

constexpr bool new_line(char c) noexcept {
	return c == '\n' || c == '\r';
}

constexpr bool quote(char c) noexcept {
	return c == '\'' || c == '"';
}

location_t update(std::string_view text, std::size_t begin, std::size_t size, location_t loc) {
	location_t ret = loc;
	for (std::size_t i = 0; i < size; ++i) {
		if (new_line(text[begin + i])) {
			++ret.line;
			ret.col = 0;
		} else {
			++ret.col;
		}
	}
	return ret;
}

constexpr bool is_escaped(std::string_view text, std::size_t this_char, char escape = '\\') noexcept {
	bool b_escaping = false;
	for (; this_char > 0 && text[this_char - 1] == escape; --this_char) {
		b_escaping = !b_escaping;
	}
	return b_escaping;
}

constexpr bool comment(char current, char next) noexcept {
	return current == '#' || (current == '/' && next == '/');
}
} // namespace

struct lexer::substring_t {
	std::string_view text;
	std::size_t index = 0;

	char peek(std::size_t offset = 0) const noexcept {
		if (index + offset < text.size()) {
			return text[index + offset];
		}
		return '\0';
	}

	location_t advance(std::size_t size, location_t loc) noexcept {
		if (index + size <= text.size()) {
			index += size;
			return update(text, index - size, size, loc);
		}
		return loc;
	}

	location_t trim(location_t loc) noexcept {
		while (remain() > 0 && std::isspace(peek())) {
			loc = advance(1, loc);
		}
		return loc;
	}

	location_t seek_new_line(location_t loc) noexcept {
		while (remain() > 0 && !new_line(peek())) {
			loc = advance(1, loc);
		}
		return loc;
	}

	constexpr bool test(char ch) const noexcept {
		return index < text.size() && text[index] == ch;
	}

	constexpr std::size_t remain() const noexcept {
		return text.size() > index ? text.size() - index : 0;
	}
};

std::optional<token_t> lexer::advance() {
	std::optional<token_t> ret;
	if (!m_next.empty()) {
		auto lex = m_next.front();
		m_next.pop_front();
		ret = lex.token;
		m_index = lex.next_idx;
		m_loc = lex.next_loc;
		return ret;
	}
	if (m_index >= m_text.size()) {
		return std::nullopt;
	}
	auto [t, l, i] = next();
	m_loc = l;
	m_index = i;
	return t;
}

std::vector<token_t> lexer::peek(std::size_t count) {
	std::vector<token_t> ret;
	if (count <= m_next.size()) {
		ret.reserve(count);
		for (auto const& scan : m_next) {
			ret.push_back(scan.token);
		}
		return ret;
	}
	std::size_t index = m_next.empty() ? m_index : m_next.back().next_idx;
	for (; index < m_text.size() && m_next.size() < count; index = m_next.back().next_idx) {
		m_next.push_back(next());
	}
	ret.reserve(m_next.size());
	for (auto const& scan : m_next) {
		ret.push_back(scan.token);
	}
	return ret;
}

lexer::scan_t lexer::next() {
	std::optional<token_t> tk;
	std::size_t begin = m_next.empty() ? m_index : m_next.back().next_idx;
	location_t loc = m_next.empty() ? m_loc : m_next.back().next_loc;
	substring_t sub{m_text, begin};
	if (quote(sub.peek())) {
		loc = sub.advance(1, loc);
		auto ret = make_value(sub, ++begin, loc, [](substring_t const& s) { return quote(s.peek()) && !is_escaped(s.text, s.index); });
		sub.index = ret.next_idx;
		assert(quote(sub.peek()));
		loc = sub.advance(1, ret.next_loc);
		auto const [l, i] = seek(loc, sub.index);
		return {ret.token, l, i};
	}
	if (comment(sub.peek(), sub.peek(1))) {
		loc = sub.seek_new_line(loc);
		loc = sub.trim(loc);
		begin = sub.index;
	}
	if (tk = match(sub, loc); tk) {
		auto lnext = sub.advance(tk->text.size(), tk->loc);
		lnext = sub.trim(lnext);
		auto const [l, i] = seek(lnext, sub.index);
		return scan_t{*tk, l, i};
	}
	return make_value(sub, begin, loc, [this](substring_t const& s) { return std::isspace(s.peek()) || match(s, {}).has_value(); });
}

std::optional<token_t> lexer::match(substring_t const& sub, location_t const& loc) const {
	if (sub.index < m_text.size()) {
		if (auto it = char_tokens.find(sub.peek()); it != char_tokens.end()) {
			return make_token(sub.index, 1, loc, it->second);
		}
	}
	return std::nullopt;
}

std::pair<location_t, size_t> lexer::seek(location_t const& loc, std::size_t end) const {
	if (end >= m_text.size()) {
		return {loc, m_text.size()};
	}
	substring_t sub{m_text, end};
	location_t l = loc;
	if (comment(sub.peek(), sub.peek(1))) {
		l = sub.seek_new_line(l);
	}
	l = sub.trim(l);
	return {l, sub.index};
}

token_t lexer::make_token(std::size_t begin, std::size_t length, location_t const& loc, tk_type type) const {
	return token_t{std::string_view(m_text.data() + begin, length), loc, type};
}

template <typename F>
lexer::scan_t lexer::make_value(substring_t sub, std::size_t begin, location_t const& loc, F pred) const {
	location_t next_loc = loc;
	while (sub.remain() > 0) {
		next_loc = sub.advance(1, next_loc);
		if (pred(sub)) {
			break;
		}
	}
	auto const [l, i] = seek(next_loc, sub.index);
	return scan_t{make_token(begin, sub.index - begin, loc, tk_type::value), l, i};
}
} // namespace dj::detail
