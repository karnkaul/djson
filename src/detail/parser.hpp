#pragma once
#include <detail/scanner.hpp>
#include <djson/json.hpp>
#include <iostream>
#include <sstream>

namespace dj::detail {
namespace {
struct factory;
} // namespace

template <>
struct facade<factory> {
	json operator()(value_t&& value) const { return json(std::move(value)); }
};

static json make_json(value_t&& value) { return facade<factory>{}(std::move(value)); }

struct error_t {
	std::string text{};
	loc_t loc{};
	tk_type_t token_type{};

	error_t(scan_t scan) : text(scan.token.text), loc(scan.loc), token_type(scan.token.type) {}
};

inline val_t clone(val_t const& rhs) {
	auto ret = val_t{};
	struct visitor {
		val_t& ret;
		void operator()(lit_t const& lit) { ret = lit; }
		void operator()(arr_t const& arr) {
			auto copy = arr_t{};
			for (auto const& val : arr.nodes) { copy.nodes.push_back(std::make_unique<json>(*val)); }
			ret = std::move(copy);
		}
		void operator()(obj_t const& obj) {
			auto copy = obj_t{};
			for (auto const& [k, v] : obj.nodes) { copy.nodes.insert_or_assign(k, std::make_unique<json>(*v)); }
			ret = std::move(copy);
		}
	};
	std::visit(visitor{ret}, rhs);
	return ret;
}

struct parser_t {
	using tt = tk_type_t;

	scanner_t scanner{};
	scan_t current{};

	static bool is_digit(char const ch) { return std::isdigit(static_cast<unsigned char>(ch)); }

	static std::string unquote(std::string_view text) {
		auto str = std::stringstream{};
		text = text.substr(1, text.size() - 2);
		auto const size = text.size();
		for (std::size_t i{0}; i < size; ++i) {
			char ch = text[i];
			if (ch == escape_v && i + 1 < size) {
				char const next = text[i + 1];
				++i;
				switch (next) {
				case 'n': ch = '\n'; break;
				case 't': ch = '\t'; break;
				case 'b': continue;
				default: ch = next; break;
				}
			}
			str << ch;
		}
		return str.str();
	}

	parser_t(std::string_view text) : scanner{text} {}

	value_t parse() {
		if (!scanner.advance(current)) { return {}; }
		auto ret = make_value();
		ensure(tt::eof);
		return ret;
	}

	void ensure(tt type) const {
		if (current.token.type != type) { throw error_t(current); }
	}

	void consume(tt type) {
		ensure(type);
		scanner.advance(current);
	}

	template <typename T>
	T make_and_adv(T t) {
		scanner.advance(current);
		return t;
	}

	std::string make_unquoted() {
		ensure(tt::unquoted);
		return make_and_adv(std::string(current.token.text));
	}

	std::string make_quoted() {
		ensure(tt::quoted);
		return make_and_adv(unquote(current.token.text));
	}

	arr_t make_array() {
		consume(tt::lbracket);
		auto ret = arr_t{};
		while (current.token.type != tt::rbracket) {
			ret.nodes.push_back(std::make_unique<json>(make_json(make_value())));
			if (current.token.type == tt::rbracket) { break; }
			consume(tt::comma);
		}
		consume(tt::rbracket);
		return ret;
	}

	obj_t make_obj() {
		consume(tt::lbrace);
		auto ret = obj_t{};
		while (current.token.type != tt::rbrace) {
			if (current.token.type != tt::quoted) { throw error_t(current); }
			auto const loc = current.loc;
			auto key = make_quoted();
			if (key.empty()) { throw error_t(scan_t{{"(empty)", tt::quoted}, loc}); }
			consume(tt::colon);
			ret.nodes.insert_or_assign(std::move(key), std::make_unique<json>(make_json(make_value())));
			if (current.token.type == tt::rbrace) { break; }
			consume(tt::comma);
		}
		consume(tt::rbrace);
		return ret;
	}

	value_type require_number() const {
		auto text = current.token.text;
		if (text[0] == '-') { text = text.substr(1); }					// ignore minus
		if (text.empty() || text[0] == '.') { throw error_t{current}; } // require digit before decimal
		float discard;
		auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), discard);
		if (ec != std::errc() || ptr != text.data() + text.size()) { throw error_t{current}; }
		return value_type::number;
	}

	value_type literal_type() const {
		auto text = current.token.text;
		if (text.empty() || text == "null") { return value_type::null; }
		if (text == "true" || text == "false") { return value_type::boolean; }
		return require_number();
	}

	value_t make_literal() {
		auto const type = literal_type();
		return {make_unquoted(), type};
	}

	value_t make_value() {
		switch (current.token.type) {
		case tt::lbrace: return {make_obj(), value_type::object};
		case tt::lbracket: return {make_array(), value_type::array};
		case tt::unquoted: return make_literal();
		case tt::quoted: return {make_quoted(), value_type::string};
		default: throw error_t(current);
		}
	}
};
} // namespace dj::detail
