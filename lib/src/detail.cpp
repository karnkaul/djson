module;

#include <cassert>

module djson.detail;

namespace dj::detail {
namespace fs = std::filesystem;

namespace {
[[nodiscard]] constexpr auto to_parse_error_type(ScanError::Type const type) {
	switch (type) {
	case ScanError::Type::MissingClosingQuote: return Error::Type::MissingClosingQuote;
	case ScanError::Type::UnrecognizedToken: return Error::Type::UnrecognizedToken;
	case ScanError::Type::MissingEndComment: return Error::Type::MissingEndComment;
	default: return Error::Type::Unknown;
	}
}

[[nodiscard]] constexpr auto to_parse_error(ScanError const err) {
	return Error{
		.type = to_parse_error_type(err.type),
		.token = std::string{err.token},
		.src_loc = err.src_loc,
	};
}

[[nodiscard]] constexpr auto has_decimal_or_exponent(token::Number const& num) {
	return num.raw_str.find_first_of(".eE") != std::string_view::npos;
}

[[nodiscard]] constexpr auto is_negative(token::Number const& num) { return num.raw_str.starts_with('-'); }

struct Unescape {
	[[nodiscard]] auto operator()(std::string& out) -> std::optional<Error::Type> {
		for (char const c : in.escaped) {
			if (escape) {
				if (c == 'u') {
					return Error::Type::UnsupportedFeature;
				}
				if (!unescape(out, c)) {
					return Error::Type::InvalidEscape;
				}
				escape = false;
				continue;
			}
			if (c == '\\' && !escape) {
				escape = true;
				continue;
			}
			out.push_back(c);
		}
		if (escape) {
			return Error::Type::InvalidEscape;
		}
		return {};
	}

	[[nodiscard]] static auto unescape(std::string& out, char const escaped) -> bool {
		switch (escaped) {
		case '\"': out.push_back('\"'); return true;
		case '\\': out.push_back('\\'); return true;
		case '/': out.push_back('/'); return true;
		case 'b': {
			if (!out.empty()) {
				out.pop_back();
			}
			return true;
		}
		case 'f': {
			// TODO: WTF is form feed
			return true;
		}
		case 'n': out.push_back('\n'); return true;
		case 'r': out.push_back('\r'); return true;
		case 't': out.push_back('\t'); return true;
		default: return false;
		}
	}

	token::String in{};

	bool escape{};
};
} // namespace

auto Parser::make_json(Value::Payload payload) -> Json { return create(std::make_unique<Value>(std::move(payload))); }

Parser::Parser(std::string_view const text, ParseMode const mode) : m_mode(mode), m_scanner(text) {}

auto Parser::parse() -> Result<Json> {
	try {
		check_jsonc_header();
		if (m_current.is<token::Eof>()) {
			return null_json_v;
		}

		auto ret = parse_value();
		if (!m_current.is<token::Eof>()) {
			throw make_error(Error::Type::UnexpectedToken);
		}

		return ret;
	} catch (Error const& err) {
		return std::unexpected(err);
	}
	return null_json_v;
}

auto Parser::next_token() -> Token {
	auto scan_result = m_scanner.next();
	if (!scan_result) {
		throw to_parse_error(scan_result.error());
	}
	return *scan_result;
}

auto Parser::next_non_comment() -> Token {
	auto const is_comment = [this](Token const& token) {
		if (!token.is<token::Comment>()) {
			return false;
		}
		handle_comment(token);
		return true;
	};

	auto ret = Token{};
	do {
		ret = next_token();
	} while (is_comment(ret));
	return ret;
}

void Parser::handle_comment(Token const& token) const {
	if (m_mode == ParseMode::Strict) {
		throw make_error(token, Error::Type::UnexpectedComment);
	}
}

void Parser::advance() {
	m_current = m_next;
	m_next = next_non_comment();
}

void Parser::consume(token::Operator const expected, Error::Type const on_error) {
	if (!m_current.is_operator(expected)) {
		throw make_error(on_error);
	}
	advance();
}

auto Parser::make_error(Token const token, Error::Type const type) -> Error {
	return Error{
		.type = type,
		.token = std::string{token.lexeme},
		.src_loc = token.src_loc,
	};
}

auto Parser::make_error(Error::Type const type) const -> Error { return make_error(m_current, type); }

auto Parser::parse_value() -> Json {
	if (m_current.is<token::Eof>()) {
		throw make_error(Error::Type::UnexpectedEof);
	}
	if (auto const* op = std::get_if<token::Operator>(&m_current.type)) {
		return from_operator(*op);
	}
	if (auto const* num = std::get_if<token::Number>(&m_current.type)) {
		if (has_decimal_or_exponent(*num)) {
			return make_number<double>(*num);
		}
		if (is_negative(*num)) {
			return make_number<std::int64_t>(*num);
		}
		return make_number<std::uint64_t>(*num);
	}
	assert(m_current.is<token::String>()); // NOLINT(misc-static-assert)
	return make_string(std::get<token::String>(m_current.type));
}

auto Parser::from_operator(token::Operator const op) -> Json {
	auto ret = Json{};
	switch (op) {
	case token::Operator::Null: ret = null_json_v; break;
	case token::Operator::True: ret = make_json(literal::Bool{.value = true}); break;
	case token::Operator::False: ret = make_json(literal::Bool{.value = false}); break;
	case token::Operator::SquareLeft: return make_array();
	case token::Operator::BraceLeft: return make_object();

	case token::Operator::Comma:
	case token::Operator::Colon:
	case token::Operator::SquareRight:
	case token::Operator::BraceRight:
	default: throw make_error(Error::Type::UnexpectedToken);
	}

	advance();
	return ret;
}

template <typename T>
auto Parser::make_number(token::Number const in) -> Json {
	auto value = T{};
	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
	auto const* end = in.raw_str.data() + in.raw_str.size();
	auto const [ptr, ec] = std::from_chars(in.raw_str.data(), end, value);
	if (ec != std::errc{} || ptr != end) {
		throw make_error(Error::Type::InvalidNumber);
	}
	advance();
	return make_json(literal::Number{.payload = value});
}

auto Parser::make_string(token::String const in) -> Json {
	auto ret = detail::literal::String{};
	ret.text = unescape_string(in);
	advance();
	return make_json(std::move(ret));
}

auto Parser::iterate_unless(token::Operator const op) -> bool {
	if (!m_current.is_operator(token::Operator::Comma)) {
		return false;
	}
	advance();
	if (m_mode == ParseMode::Strict) {
		// require more content after ','
		return true;
	}
	if (m_current.is_operator(op)) {
		return false;
	}
	return true;
}

auto Parser::make_array() -> Json {
	assert(m_current.is_operator(token::Operator::SquareLeft)); // NOLINT(misc-static-assert)
	auto ret = Array{};
	advance();
	if (!m_current.is_operator(token::Operator::SquareRight)) {
		do {
			ret.members.push_back(parse_value());
		} while (iterate_unless(token::Operator::SquareRight));
	}
	consume(token::Operator::SquareRight, Error::Type::MissingBracket);
	return make_json(std::move(ret));
}

auto Parser::make_object() -> Json {
	assert(m_current.is_operator(token::Operator::BraceLeft)); // NOLINT(misc-static-assert)
	auto ret = Object{};
	advance();
	if (!m_current.is_operator(token::Operator::BraceRight)) {
		do {
			auto key = make_key();
			consume(token::Operator::Colon, Error::Type::MissingColon);
			auto value = parse_value();
			ret.members.insert_or_assign(std::move(key), std::move(value));
		} while (iterate_unless(token::Operator::BraceRight));
	}
	consume(token::Operator::BraceRight, Error::Type::MissingBrace);
	return make_json(std::move(ret));
}

auto Parser::unescape_string(token::String const in) const -> std::string {
	auto ret = std::string{};
	ret.reserve(in.escaped.size());
	auto const error = Unescape{.in = in}(ret);
	if (error) {
		throw make_error(*error);
	}
	return ret;
}

auto Parser::make_key() -> std::string {
	auto const* string = std::get_if<token::String>(&m_current.type);
	if (!string) {
		throw make_error(Error::Type::MissingKey);
	}
	auto ret = unescape_string(*string);
	advance();
	return ret;
}

void Parser::check_jsonc_header() {
	auto const token = next_token();
	if (!token.is<token::Comment>()) {
		if (m_mode == ParseMode::Auto) {
			m_mode = ParseMode::Strict;
		}
		m_current = token;
		m_next = next_non_comment();
		return;
	}

	handle_comment(token);

	if (m_mode == ParseMode::Auto) {
		static constexpr auto jsonc_headers_v = std::array{
			"// -*- mode: jsonc -*-"sv,
			"// -*- jsonc -*-"sv,
		};
		if (std::ranges::find(jsonc_headers_v, token.lexeme) != jsonc_headers_v.end()) {
			m_mode = ParseMode::Jsonc;
		} else {
			m_mode = ParseMode::Strict;
		}
	}
	m_current = next_non_comment();
	m_next = next_non_comment();
}
} // namespace dj::detail
