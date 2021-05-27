#include <detail/tokenizer.hpp>

namespace dj::detail {
loc_t lexeme_t::loc(std::string_view full) const noexcept {
	if (begin > full.size()) { return {0, 0}; }
	loc_t ret;
	for (std::size_t i = 0; i < begin; ++i) {
		if (full[i] == '\n') {
			++ret.line;
			ret.col = 0;
		}
		++ret.col;
	}
	return ret;
}

std::string_view lexeme_t::str(std::string_view full) const noexcept { return begin < full.size() ? full.substr(begin, size) : "[EOF]"; }

token_t tokenizer_t::scan() {
	token_t ret;
	if (m_idx >= m_text.size()) { return eof(m_idx); }
	while (std::isspace(peek())) { advance(); }
	ret.lexeme.begin = m_idx;
	fill_token(ret);
	advance(ret.lexeme.size);
	return ret;
}

void tokenizer_t::reserved(token_t& out, std::string_view str, tk_type type) const noexcept {
	if (expect(str)) {
		out.type = type;
		out.lexeme.size = str.size();
	} else {
		out.type = tk_type::unknown;
	}
}

bool tokenizer_t::escaped(std::size_t idx) const noexcept {
	bool ret = false;
	for (; idx > 0 && m_text[idx - 1] == '\\'; --idx) { ret = !ret; }
	return ret;
}

void tokenizer_t::munch(token_t& out) const noexcept {
	char const c = peek();
	if (c == '\"') {
		if (remain() < 1) {
			out.type = tk_type::unknown;
			return;
		}
		out.type = tk_type::string;
		std::size_t end = out.lexeme.begin + 1;
		for (; end < m_text.size(); ++end) {
			if (!escaped(end) && m_text[end] == '\"') break;
		}
		if (end >= m_text.size() || m_text[end] != '\"') {
			out.type = tk_type::unknown;
			--end;
		}
		out.lexeme.size = end + 1 - out.lexeme.begin;
	} else if (c == '-' || std::isdigit(c)) {
		out.type = tk_type::eint;
		if (c == '-' && remain() < 1) {
			out.type = tk_type::unknown;
			return;
		}
		std::size_t end = out.lexeme.begin + 1;
		for (; end < m_text.size(); ++end) {
			if (m_text[end] == '.') {
				if (out.type == tk_type::eint) {
					out.type = tk_type::efloat;
					continue;
				} else {
					break;
				}
			}
			if (!std::isdigit(m_text[end])) { break; }
		}
		out.lexeme.size = end - out.lexeme.begin;
	} else {
		out.type = tk_type::unknown;
	}
}

void tokenizer_t::fill_token(token_t& out) const noexcept {
	switch (peek()) {
	case '{': out.type = tk_type::curlyl; break;
	case '}': out.type = tk_type::curlyr; break;
	case '[': out.type = tk_type::squarel; break;
	case ']': out.type = tk_type::squarer; break;
	case ',': out.type = tk_type::comma; break;
	case ':': out.type = tk_type::colon; break;
	case 't': reserved(out, "true", tk_type::etrue); break;
	case 'f': reserved(out, "false", tk_type::efalse); break;
	case 'n': reserved(out, "null", tk_type::null); break;
	default: munch(out); break;
	}
}
} // namespace dj::detail
