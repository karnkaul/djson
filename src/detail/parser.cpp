#include <algorithm>
#include <iostream>
#include <sstream>
#include <detail/parser.hpp>
#include <dumb_json/json.hpp>

namespace dj::detail {
namespace {
constexpr bool eof(token_t const& token) noexcept { return token.type == tk_type::eof; }

std::string unescape(std::string_view text) {
	if (text.empty()) { return {}; }
	if (text.front() == '\"') {
		if (text.size() < 2 || text.back() != '\"') { return {}; }
		text = text.substr(1, text.size() - 2);
	}
	if (text.empty()) { return {}; }
	std::stringstream str;
	bool esc = false;
	for (char const c : text) {
		if (c == '\\' && !esc) {
			esc = true;
			continue;
		}
		if (esc) {
			switch (c) {
			case 'b':
				if (str.peek() != '\0') str.get();
				break;
			case 'n': str << '\n'; break;
			case 't': str << '\t'; break;
			default: str << c; break;
			}
		} else {
			str << c;
		}
		esc = false;
	}
	return str.str();
}
} // namespace

parser_t::parser_t(std::string_view text) : m_scanner(text), m_text(text) { advance(); }

bool parser_t::check(tk_list type) const noexcept {
	return std::any_of(type.begin(), type.end(), [l = m_curr.type](auto r) { return l == r; });
}

bool parser_t::expect(tk_list exp, int adv) noexcept {
	if (!check(exp)) {
		push_error(exp, (adv & adv_t::un) == adv_t::un);
		return false;
	}
	if ((adv & adv_t::on) == adv_t::on) { advance(); }
	return true;
}

void parser_t::advance() noexcept {
	m_prev = m_curr;
	m_curr = m_scanner.scan();
}

template <>
boolean_t parser_t::to<boolean_t>() {
	advance();
	return {m_prev.type == tk_type::etrue};
}

template <>
number_t parser_t::to<number_t>() {
	advance();
	return {std::string(m_prev.lexeme.str(m_text))};
}

template <>
string_t parser_t::to<string_t>() {
	advance();
	return {unescape(m_prev.lexeme.str(m_text))};
}

template <>
array_t parser_t::to<array_t>() {
	array_t ret;
	if (expect({tk_type::squarel})) {
		while (!check({tk_type::squarer, tk_type::eof})) {
			ret.value.push_back(value());
			check({tk_type::squarer}) || expect({tk_type::comma, tk_type::squarer}, adv_all);
		}
		expect({tk_type::squarer});
	}
	return ret;
}

template <>
object_t parser_t::to<object_t>() {
	object_t ret;
	if (expect({tk_type::curlyl})) {
		while (!check({tk_type::curlyr, tk_type::eof})) {
			if (expect({tk_type::string}, adv_t::un)) {
				string_t key = to<string_t>();
				if (expect({tk_type::colon})) { ret.value[std::move(key.value)] = value(); }
				check({tk_type::curlyr}) || expect({tk_type::comma, tk_type::curlyr}, adv_all);
			}
		}
		expect({tk_type::curlyr});
	}
	return ret;
}

ptr<json_t> parser_t::value() {
	ptr<json_t> ret;
	switch (m_curr.type) {
	case tk_type::null: ret = make<json_t>(null_t{}); break;
	case tk_type::curlyl: ret = make<json_t>(to<object_t>()); break;
	case tk_type::squarel: ret = make<json_t>(to<array_t>()); break;
	case tk_type::string: ret = make<json_t>(to<string_t>()); break;
	case tk_type::etrue:
	case tk_type::efalse: {
		ret = make<json_t>(to<boolean_t>());
		break;
	}
	case tk_type::eint:
	case tk_type::efloat: {
		ret = make<json_t>(to<number_t>());
		break;
	}
	default: {
		push_error({}, false);
		ret = make<json_t>(null_t{});
		break;
	}
	}
	return ret;
}

parser_t::result_t parser_t::parse() {
	m_unexpected.clear();
	result_t ret;
	ret.json = value();
	advance();
	while (!eof(m_curr)) { push_error({tk_type::eof}); }
	ret.unexpected = std::move(m_unexpected);
	return ret;
}

void parser_t::push_error(tk_list expect, bool adv) {
	m_unexpected.push_back({m_curr.lexeme.loc(m_text), m_curr.lexeme.str(m_text), {expect.begin(), expect.end()}, m_curr.type});
	if (adv) { advance(); }
}
} // namespace dj::detail
