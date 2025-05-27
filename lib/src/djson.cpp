#include <detail/parser.hpp>
#include <detail/visitor.hpp>
#include <charconv>
#include <filesystem>
#include <format>
#include <fstream>

// error

namespace dj {
namespace {
using namespace std::string_view_literals;

constexpr auto error_type_str_v = std::array{
	"Unknown error"sv,
	"Unrecognized Token"sv,
	"Missing closing quote"sv,
	"Invalid number"sv,
	"Invalid escape"sv,
	"Unexpected token"sv,
	"Unexpected comment"sv,
	"Unexpected end of file"sv,
	"Missing key"sv,
	"Missing colon (':')"sv,
	"Missing closing brace ('}')"sv,
	"Missing closing square bracket (']')"sv,
	"Missing end comment ('*/')"sv,
	"I/O error"sv,
	"Unsupported feature"sv,
};

static_assert(error_type_str_v.size() == std::size_t(Error::Type::COUNT_));
} // namespace
} // namespace dj

auto dj::to_string_view(Error::Type const type) -> std::string_view {
	if (int(type) < 0 || type >= Error::Type::COUNT_) { return dj::error_type_str_v[std::size_t(Error::Type::Unknown)]; }
	return dj::error_type_str_v.at(std::size_t(type));
}

auto dj::to_string(Error const& error) -> std::string {
	auto ret = std::string{to_string_view(error.type)};
	if (!error.token.empty()) { std::format_to(std::back_inserter(ret), " - '{}'", error.token); }
	auto const src_loc = error.src_loc;
	if (src_loc.line > 0 && src_loc.column > 0) { std::format_to(std::back_inserter(ret), " [{}:{}]", src_loc.line, src_loc.column); }
	return ret;
}

// parser

namespace dj::detail {
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

[[nodiscard]] constexpr auto has_decimal_or_exponent(token::Number const& num) { return num.raw_str.find_first_of(".eE") != std::string_view::npos; }

[[nodiscard]] constexpr auto is_negative(token::Number const& num) { return num.raw_str.starts_with('-'); }

struct Unescape {
	[[nodiscard]] auto operator()(std::string& out) -> std::optional<Error::Type> {
		for (char const c : in.escaped) {
			if (escape) {
				if (c == 'u') { return Error::Type::UnsupportedFeature; }
				if (!unescape(out, c)) { return Error::Type::InvalidEscape; }
				escape = false;
				continue;
			}
			if (c == '\\' && !escape) {
				escape = true;
				continue;
			}
			out.push_back(c);
		}
		if (escape) { return Error::Type::InvalidEscape; }
		return {};
	}

	[[nodiscard]] static auto unescape(std::string& out, char const escaped) -> bool {
		switch (escaped) {
		case '\"': out.push_back('\"'); return true;
		case '\\': out.push_back('\\'); return true;
		case '/': out.push_back('/'); return true;
		case 'b': {
			if (!out.empty()) { out.pop_back(); }
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

auto const null_json_v = dj::Json{};
} // namespace

auto Parser::make_json(Value::Payload payload) -> Json {
	auto ret = Json{};
	// NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
	ret.m_value.reset(new detail::Value{.payload = std::move(payload)});
	return ret;
}

Parser::Parser(std::string_view const text, ParseMode const mode) : m_mode(mode), m_scanner(text) {}

auto Parser::parse() -> Result {
	try {
		check_jsonc_header();
		if (m_current.is<token::Eof>()) { return null_json_v; }

		auto ret = parse_value();
		if (!m_current.is<token::Eof>()) { throw make_error(Error::Type::UnexpectedToken); }

		return ret;
	} catch (Error const& err) { return std::unexpected(err); }
	return null_json_v;
}

auto Parser::next_token() -> Token {
	auto scan_result = m_scanner.next();
	if (!scan_result) { throw to_parse_error(scan_result.error()); }
	return *scan_result;
}

auto Parser::next_non_comment() -> Token {
	auto const is_comment = [this](Token const& token) {
		if (!token.is<token::Comment>()) { return false; }
		handle_comment(token);
		return true;
	};

	auto ret = Token{};
	do { ret = next_token(); } while (is_comment(ret));
	return ret;
}

void Parser::handle_comment(Token const& token) const {
	if (m_mode == ParseMode::Strict) { throw make_error(token, Error::Type::UnexpectedComment); }
}

void Parser::advance() {
	m_current = m_next;
	m_next = next_non_comment();
}

void Parser::consume(token::Operator const expected, Error::Type const on_error) {
	if (!m_current.is_operator(expected)) { throw make_error(on_error); }
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
	if (m_current.is<token::Eof>()) { throw make_error(Error::Type::UnexpectedEof); }
	if (auto const* op = std::get_if<token::Operator>(&m_current.type)) { return from_operator(*op); }
	if (auto const* num = std::get_if<token::Number>(&m_current.type)) {
		if (has_decimal_or_exponent(*num)) { return make_number<double>(*num); }
		if (is_negative(*num)) { return make_number<std::int64_t>(*num); }
		return make_number<std::uint64_t>(*num);
	}
	assert(m_current.is<token::String>());
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
	auto const* end = in.raw_str.data() + in.raw_str.size();
	auto const [ptr, ec] = std::from_chars(in.raw_str.data(), end, value);
	if (ec != std::errc{} || ptr != end) { throw make_error(Error::Type::InvalidNumber); }
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
	if (!m_current.is_operator(token::Operator::Comma)) { return false; }
	advance();
	if (m_mode == ParseMode::Strict) {
		// require more content after ','
		return true;
	}
	if (m_current.is_operator(op)) { return false; }
	return true;
}

auto Parser::make_array() -> Json {
	assert(m_current.is_operator(token::Operator::SquareLeft));
	auto ret = Array{};
	advance();
	if (!m_current.is_operator(token::Operator::SquareRight)) {
		do { ret.members.push_back(parse_value()); } while (iterate_unless(token::Operator::SquareRight));
	}
	consume(token::Operator::SquareRight, Error::Type::MissingBracket);
	return make_json(std::move(ret));
}

auto Parser::make_object() -> Json {
	assert(m_current.is_operator(token::Operator::BraceLeft));
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
	if (error) { throw make_error(*error); }
	return ret;
}

auto Parser::make_key() -> std::string {
	auto const* string = std::get_if<token::String>(&m_current.type);
	if (!string) { throw make_error(Error::Type::MissingKey); }
	auto ret = unescape_string(*string);
	advance();
	return ret;
}

void Parser::check_jsonc_header() {
	auto const token = next_token();
	if (!token.is<token::Comment>()) {
		if (m_mode == ParseMode::Auto) { m_mode = ParseMode::Strict; }
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

// json

namespace dj {
namespace fs = std::filesystem;

namespace {
template <JsonType T>
using value_payload_type = std::variant_alternative_t<std::size_t(T) - 1, detail::Value::Payload>;

static_assert(std::same_as<value_payload_type<JsonType::Boolean>, detail::literal::Bool>);
static_assert(std::same_as<value_payload_type<JsonType::Number>, detail::literal::Number>);
static_assert(std::same_as<value_payload_type<JsonType::String>, detail::literal::String>);
static_assert(std::same_as<value_payload_type<JsonType::Array>, detail::Array>);
static_assert(std::same_as<value_payload_type<JsonType::Object>, detail::Object>);

template <typename T>
[[nodiscard]] constexpr auto to_number(detail::literal::Number const in) {
	auto const visitor = [](auto const n) { return static_cast<T>(n); };
	return std::visit(visitor, in.payload);
}

[[nodiscard]] auto file_to_string(std::string_view const path, std::string& out) {
	if (path.empty()) { return false; }

	auto const fs_path = fs::path{path};
	auto err = std::error_code{};
	if (fs::is_directory(fs_path, err) || fs::is_symlink(fs_path, err)) { return false; }

	auto file = std::ifstream{fs_path, std::ios::binary | std::ios::ate};
	if (!file.is_open()) { return false; }

	auto const size = file.tellg();
	file.seekg(0, std::ios::beg);
	out.resize(std::size_t(size));
	return !!file.read(out.data(), size);
}

[[nodiscard]] auto string_to_file(std::string_view const path, std::string_view const text) {
	if (path.empty()) { return false; }

	auto const fs_path = fs::path{path};
	auto err = std::error_code{};
	auto const parent_path = fs_path.parent_path();
	if (!parent_path.empty() && !fs::is_directory(parent_path, err)) {
		if (!fs::create_directories(parent_path, err)) { return false; }
	}

	auto file = std::ofstream{fs_path};
	return !!(file << text);
}

auto const empty_object_v = detail::Object{};
} // namespace

struct Json::Serializer {
	explicit Serializer(SerializeOptions const& options) : m_options(options) {}

	[[nodiscard]] auto operator()(Json const& json) -> std::string {
		process(json);
		if (!m_ret.empty()) {
			// pop last comma
			m_ret.pop_back();
			if (!is_set(Flag::NoSpaces) && is_set(Flag::TrailingNewline)) { m_ret.push_back('\n'); }
		}
		return std::move(m_ret);
	}

  private:
	using Flag = SerializeFlag;

	[[nodiscard]] constexpr auto is_set(SerializeFlags const flag) const -> bool { return (m_options.flags & flag) == flag; }

	void process(dj::Json const& json) {
		if (json.is_null()) {
			append("null,");
			return;
		}

		auto const visitor = detail::Visitor{
			[this](detail::literal::Bool const b) { append("{},", b.value); },
			[this](detail::literal::Number const n) { std::visit([this](auto const n) { append("{},", n); }, n.payload); },
			[this](detail::literal::String const& s) { append(R"("{}",)", s.text); },
			[this](detail::Array const& a) { process_array(a); },
			[this](detail::Object const& o) { process_object(o); },
		};
		std::visit(visitor, json.m_value->payload);
	}

	void process_array(detail::Array const& array) {
		if (array.members.empty()) {
			m_ret.append("[],");
			return;
		}

		m_ret.push_back('[');
		++m_indents;
		for (auto const& json : array.members) {
			pre_next_value();
			process(json);
		}
		// pop last comma
		m_ret.pop_back();
		--m_indents;
		newline();
		m_ret.append("],");
	}

	void process_object(detail::Object const& object) {
		if (object.members.empty()) {
			m_ret.append("{},");
			return;
		}

		if (is_set(Flag::SortKeys)) {
			m_sorted_keys.clear();
			m_sorted_keys.reserve(object.members.size());
			for (auto const& [key, _] : object.members) { m_sorted_keys.push_back(key); }
			std::ranges::sort(m_sorted_keys);
		}

		m_ret.push_back('{');
		++m_indents;

		if (is_set(Flag::SortKeys)) {
			for (auto const key : m_sorted_keys) {
				auto const it = object.members.find(key);
				assert(it != object.members.end());
				subprocess_object(key, it->second);
			}
		} else {
			for (auto const& [key, value] : object.members) { subprocess_object(key, value); }
		}

		// pop last comma
		m_ret.pop_back();
		--m_indents;
		newline();
		m_ret.append("},");
	}

	void subprocess_object(std::string_view const key, Json const& value) {
		pre_next_value();
		append(R"("{}":)", key);
		space();
		process(value);
	}

	template <typename... Args>
	void append(std::format_string<Args...> fmt, Args&&... args) {
		std::format_to(std::back_inserter(m_ret), fmt, std::forward<Args>(args)...);
	}

	void space() {
		if (is_set(Flag::NoSpaces)) { return; }
		m_ret.push_back(' ');
	}

	void newline() {
		if (is_set(Flag::NoSpaces)) { return; }
		m_ret.append(m_options.newline);
		for (std::uint8_t i = 0; i < m_indents; ++i) { m_ret.append(m_options.indent); }
	}

	void pre_next_value() {
		if (m_ret.empty() || is_set(Flag::NoSpaces)) { return; }
		newline();
	}

	SerializeOptions const& m_options;

	std::string m_ret{};
	std::uint8_t m_indents{};
	std::vector<std::string_view> m_sorted_keys{};
};

void Json::Deleter::operator()(detail::Value* ptr) const noexcept { std::default_delete<detail::Value>{}(ptr); }

Json::Json(Json const& other) {
	// NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
	if (other.m_value) { m_value.reset(new detail::Value{*other.m_value}); }
}

auto Json::operator=(Json const& other) -> Json& {
	if (&other != this) {
		if (!other.m_value) {
			m_value.reset();
		} else {
			ensure_impl();
			*m_value = *other.m_value;
		}
	}
	return *this;
}

auto Json::parse(std::string_view const text, ParseMode const mode) -> Result { return detail::Parser{text, mode}.parse(); }

auto Json::from_file(std::string_view const path, ParseMode const mode) -> Result {
	auto text = std::string{};
	if (!file_to_string(path, text)) { return std::unexpected(Error{.type = Error::Type::IoError}); }
	return parse(text, mode);
}

auto Json::empty_array() -> Json const& {
	static auto const ret = detail::Parser::make_json(detail::Array{});
	return ret;
}

auto Json::empty_object() -> Json const& {
	static auto const ret = detail::Parser::make_json(detail::Object{});
	return ret;
}

auto Json::get_type() const -> Type {
	if (m_value) { return Type(m_value->payload.index() + 1); }
	return Type::Null;
}

auto Json::as_bool(bool const fallback) const -> bool {
	if (!is_boolean()) { return fallback; }
	return std::get<detail::literal::Bool>(m_value->payload).value;
}

auto Json::as_double(double const fallback) const -> double {
	if (!is_number()) { return fallback; }
	return to_number<double>(std::get<detail::literal::Number>(m_value->payload));
}

auto Json::as_u64(std::uint64_t const fallback) const -> std::uint64_t {
	if (!is_number()) { return fallback; }
	return to_number<std::uint64_t>(std::get<detail::literal::Number>(m_value->payload));
}

auto Json::as_i64(std::int64_t const fallback) const -> std::int64_t {
	if (!is_number()) { return fallback; }
	return to_number<std::int64_t>(std::get<detail::literal::Number>(m_value->payload));
}

auto Json::as_string_view(std::string_view const fallback) const -> std::string_view {
	if (!is_string()) { return fallback; }
	return std::get<detail::literal::String>(m_value->payload).text;
}

auto Json::as_array() const -> std::span<Json const> {
	if (!is_array()) { return {}; }
	return std::get<detail::Array>(m_value->payload).members;
}

auto Json::as_object() const -> StringTable<Json> const& {
	if (!is_object()) { return empty_object_v.members; }
	return std::get<detail::Object>(m_value->payload).members;
}

void Json::set_null() { m_value.reset(); }

void Json::set_boolean(bool const value) {
	ensure_impl();
	m_value->payload = detail::literal::Bool{.value = value};
}

void Json::set_string(std::string_view const value) {
	ensure_impl();
	m_value->payload = detail::literal::String{.text = make_escaped(value)};
}

void Json::set_number(std::int64_t const value) {
	ensure_impl();
	m_value->payload = detail::literal::Number{.payload = value};
}

void Json::set_number(std::uint64_t const value) {
	ensure_impl();
	m_value->payload = detail::literal::Number{.payload = value};
}

void Json::set_number(double const value) {
	ensure_impl();
	m_value->payload = detail::literal::Number{.payload = value};
}

void Json::set_value(Json value) { m_value = std::move(value.m_value); }

void Json::set_array() {
	ensure_impl();
	m_value->morph<detail::Array>().members.clear();
}

void Json::set_object() {
	ensure_impl();
	m_value->morph<detail::Object>().members.clear();
}

auto Json::push_back(Json value) -> Json& {
	ensure_impl();
	return m_value->morph<detail::Array>().members.emplace_back(std::move(value));
}

auto Json::insert_or_assign(std::string key, Json value) -> Json& {
	ensure_impl();
	auto& table = m_value->morph<detail::Object>().members;
	auto const [it, _] = table.insert_or_assign(std::move(key), std::move(value));
	assert(it != table.end());
	return it->second;
}

auto Json::operator[](std::string_view const key) const -> Json const& {
	if (!is_object()) { return detail::null_json_v; }
	auto const& object = std::get<detail::Object>(m_value->payload);
	auto const it = object.members.find(key);
	if (it == object.members.end()) { return detail::null_json_v; }
	return it->second;
}

auto Json::operator[](std::string_view const key) -> Json& {
	ensure_impl();
	auto& object = m_value->morph<detail::Object>();
	auto it = object.members.find(key);
	if (it == object.members.end()) { it = object.members.insert_or_assign(std::string{key}, Json{}).first; }
	assert(it != object.members.end());
	return it->second;
}

auto Json::operator[](std::size_t const index) const -> Json const& {
	if (!is_array()) { return detail::null_json_v; }
	auto const& array = std::get<detail::Array>(m_value->payload);
	if (index >= array.members.size()) { return detail::null_json_v; }
	return array.members.at(index);
}

auto Json::operator[](std::size_t const index) -> Json& {
	ensure_impl();
	auto& array = m_value->morph<detail::Array>();
	if (index >= array.members.size()) { array.members.resize(index + 1); }
	return array.members.at(index);
}

auto Json::serialize(SerializeOptions const& options) const -> std::string { return Serializer{options}(*this); }

auto Json::to_file(std::string_view const path, SerializeOptions const& options) const -> bool {
	if (path.empty()) { return false; }
	auto const text = serialize(options);
	return string_to_file(path, text);
}

void Json::ensure_impl() {
	if (m_value) { return; }
	// NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
	m_value.reset(new detail::Value);
}
} // namespace dj

auto dj::make_escaped(std::string_view const text) -> std::string {
	auto ret = std::string{};
	ret.reserve(text.size());
	for (char const c : text) {
		switch (c) {
		case '\"':
		case '\\':
		case '/': ret.push_back('\\'); break;
		case '\t': ret.append("\\t"); continue;
		case '\n': ret.append("\\n"); continue;
		case '\r': ret.append("\\r"); continue;
		default: break;
		}
		ret.push_back(c);
	}
	return ret;
}

auto std::formatter<dj::Json>::format(dj::Json const& json, std::format_context& fc) -> std::format_context::iterator {
	auto const str = json.serialize(dj::SerializeOptions{.flags = dj::SerializeFlag::NoSpaces});
	return std::format_to(fc.out(), "{}", str);
}
