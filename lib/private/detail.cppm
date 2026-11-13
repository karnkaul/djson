module;

#include <cassert>

export module djson.detail;

export import djson;

#pragma region common
export namespace dj::detail {
using namespace std::string_view_literals;

template <typename... Ts>
struct Visitor : Ts... {
	using Ts::operator()...;
};
} // namespace dj::detail

#pragma endregion

#pragma region Token

export namespace dj::detail {
namespace token {
struct Eof {};

enum class Operator : std::int8_t {
	Null,
	True,
	False,
	Colon,
	Comma,
	BraceLeft,
	BraceRight,
	SquareLeft,
	SquareRight,
	COUNT_,
};
inline constexpr auto operator_str_v =
	std::array{"null"sv, "true"sv, "false"sv, ":"sv, ","sv, "{"sv, "}"sv, "["sv, "]"sv};
static_assert(operator_str_v.size() == std::size_t(Operator::COUNT_));

struct Number {
	std::string_view raw_str{};
};

struct String {
	std::string_view escaped{};
};

struct Comment {
	std::string_view text{};
};
} // namespace token

using TokenType = std::variant<token::Eof, token::Operator, token::String, token::Number, token::Comment>;

struct Token {
	using Type = TokenType;

	template <typename T>
	[[nodiscard]] constexpr auto is() const -> bool {
		return std::holds_alternative<T>(type);
	}

	[[nodiscard]] constexpr auto is_operator(token::Operator const op) const {
		if (auto const* o = std::get_if<token::Operator>(&type)) {
			return *o == op;
		}
		return false;
	}

	Type type{token::Eof{}};
	std::string_view lexeme{};
	SrcLoc src_loc{};
};
} // namespace dj::detail

#pragma endregion

#pragma region Scanner

export namespace dj::detail {
struct ScanError {
	enum class Type : std::int8_t {
		UnrecognizedToken,
		MissingClosingQuote,
		MissingEndComment,
	};

	Type type{};
	std::string_view token{};
	SrcLoc src_loc{};
};

class Scanner {
	[[nodiscard]] static constexpr auto is_space(char const c) -> bool {
		constexpr auto chars_v = std::array{' ', '\t', '\n', '\r'};
		return std::ranges::find(chars_v, c) != chars_v.end();
	}

	[[nodiscard]] static constexpr auto is_digit(char const c) -> bool { return c >= '0' && c <= '9'; }

	[[nodiscard]] static constexpr auto is_part_number(char const c) -> bool {
		constexpr auto chars_v = std::array{'.', 'e', 'E', '-', '+'};
		return is_digit(c) || std::ranges::find(chars_v, c) != chars_v.end();
	}

	constexpr void advance() {
		assert(!m_remain.empty()); // NOLINT(misc-static-assert)
		if (m_remain.front() == '\n') {
			++m_src_loc.line;
			m_src_loc.column = 1;
		} else {
			++m_src_loc.column;
		}
		m_remain.remove_prefix(1);
	}

	constexpr void trim_front() {
		while (!m_remain.empty() && is_space(m_remain.front())) {
			advance();
		}
	}

	template <typename T>
	[[nodiscard]] constexpr auto to_token(T const type, std::uint64_t const length) -> Token {
		auto const ret = Token{
			.type = type,
			.lexeme = m_remain.substr(0, length),
			.src_loc = m_src_loc,
		};
		if (length > 0) {
			for (std::uint64_t i = 0; i < length; ++i) {
				advance();
			}
			trim_front();
		}
		return ret;
	}

	[[nodiscard]] constexpr auto to_scan_error(ScanError::Type const type, std::uint64_t const length) const {
		return ScanError{
			.type = type,
			.token = m_remain.substr(0, length),
			.src_loc = m_src_loc,
		};
	}

	[[nodiscard]] constexpr auto try_operator(Token& out) -> bool {
		for (auto const [index, op_str] : std::views::enumerate(token::operator_str_v)) {
			if (!m_remain.starts_with(op_str)) {
				continue;
			}
			out = to_token(token::Operator(index), op_str.size());
			return true;
		}
		return false;
	}

	[[nodiscard]] constexpr auto try_number(Token& out) -> bool {
		if (m_remain.front() != '-' && !is_digit(m_remain.front())) {
			return false;
		}
		auto index = 1uz;
		for (; index < m_remain.size() && is_part_number(m_remain.at(index)); ++index) {}
		auto const length = index;
		out = to_token(token::Number{.raw_str = m_remain.substr(0, length)}, length);
		return true;
	}

	[[nodiscard]] constexpr auto scan_string() -> std::expected<Token, ScanError> {
		assert(m_remain.starts_with('\"')); // NOLINT(misc-static-assert)
		auto escaped = false;
		auto index = 1uz;
		for (; index < m_remain.size(); ++index) {
			if (escaped) {
				escaped = false;
				continue;
			}
			char const ch = m_remain.at(index);
			if (ch == '\\') {
				escaped = true;
				continue;
			}
			if (ch == '\"') {
				break;
			}
		}
		if (index == m_remain.size()) {
			return std::unexpected(to_scan_error(ScanError::Type::MissingClosingQuote, 1));
		}

		auto const length = index - 1;
		return to_token(token::String{.escaped = m_remain.substr(1, length)}, length + 2);
	}

	[[nodiscard]] constexpr auto scan_line_comment() -> Token {
		assert(m_remain.starts_with("//")); // NOLINT(misc-static-assert)
		auto length = m_remain.find('\n');
		if (length == std::string_view::npos) {
			length = m_remain.size();
		}
		return to_token(token::Comment{.text = m_remain.substr(0, length)}, length);
	}

	[[nodiscard]] constexpr auto scan_multiline_comment() -> std::expected<Token, ScanError> {
		assert(m_remain.starts_with("/*")); // NOLINT(misc-static-assert)
		auto length = m_remain.find("*/");
		if (length == std::string_view::npos) {
			return std::unexpected(to_scan_error(ScanError::Type::MissingEndComment, 2));
		}
		assert(length + 2 <= m_remain.size()); // NOLINT(misc-static-assert)
		length += 2;
		return to_token(token::Comment{.text = m_remain.substr(0, length)}, length);
	}

	std::string_view m_remain{};
	SrcLoc m_src_loc{};

  public:
	explicit constexpr Scanner(std::string_view const text) : m_remain(text) {
		trim_front();
		if (!m_remain.empty()) {
			m_src_loc = {.line = 1, .column = 1};
		}
	}

	[[nodiscard]] constexpr auto next() -> std::expected<Token, ScanError> {
		if (m_remain.empty()) {
			return to_token(token::Eof{}, 0);
		}

		auto ret = Token{.src_loc = m_src_loc};

		if (try_operator(ret)) {
			return ret;
		}
		if (try_number(ret)) {
			return ret;
		}
		if (m_remain.starts_with('\"')) {
			return scan_string();
		}
		if (m_remain.starts_with("//")) {
			return scan_line_comment();
		}
		if (m_remain.starts_with("/*")) {
			return scan_multiline_comment();
		}

		return std::unexpected(to_scan_error(ScanError::Type::UnrecognizedToken, 1));
	}
};
} // namespace dj::detail

#pragma endregion

#pragma region Value

export namespace dj::detail {
namespace literal {
struct Bool {
	bool value{};
};

struct Number {
	using Payload = std::variant<double, std::uint64_t, std::int64_t>;

	Payload payload{};
};

struct String {
	std::string text{};
};
} // namespace literal

struct Array {
	std::vector<dj::Json> members{};
};

struct Object {
	StringTable<dj::Json> members{};
};

class Value : public IValue {
  public:
	using Payload = std::variant<literal::Bool, literal::Number, literal::String, Array, Object>;

	explicit Value(Payload payload = {}) : payload(std::move(payload)) {}

	template <typename T>
	auto morph() -> T& {
		auto* ret = std::get_if<T>(&payload);
		if (!ret) {
			ret = &payload.emplace<T>();
		}
		return *ret;
	}

	Payload payload{};
};
} // namespace dj::detail

#pragma endregion

#pragma region Parser

export namespace dj::detail {
auto const null_json_v = dj::Json{};

class Parser : public IParser {
  public:
	[[nodiscard]] static auto make_json(Value::Payload payload) -> Json;

	explicit Parser(std::string_view text, ParseMode mode);

	[[nodiscard]] auto parse() -> Result<Json>;

  private:
	[[nodiscard]] auto next_token() -> Token;
	[[nodiscard]] auto next_non_comment() -> Token;
	void handle_comment(Token const& token) const;
	void advance();
	void consume(token::Operator expected, Error::Type on_error);

	[[nodiscard]] static auto make_error(Token token, Error::Type type) -> Error;
	[[nodiscard]] auto make_error(Error::Type type) const -> Error;

	[[nodiscard]] auto parse_value() -> Json;

	[[nodiscard]] auto from_operator(token::Operator op) -> Json;
	template <typename T>
	[[nodiscard]] auto make_number(token::Number in) -> Json;
	[[nodiscard]] auto make_string(token::String in) -> Json;

	[[nodiscard]] auto iterate_unless(token::Operator op) -> bool;
	[[nodiscard]] auto make_array() -> Json;
	[[nodiscard]] auto make_object() -> Json;

	[[nodiscard]] auto unescape_string(token::String in) const -> std::string;
	[[nodiscard]] auto make_key() -> std::string;

	void check_jsonc_header();

	ParseMode m_mode{ParseMode::Auto};

	Scanner m_scanner;
	Token m_current{};
	Token m_next{};
};
} // namespace dj::detail

#pragma endregion
