#pragma once
#include <detail/scanner.hpp>
#include <detail/value.hpp>
#include <djson/json.hpp>

namespace dj::detail {
class Parser {
  public:
	[[nodiscard]] static auto make_json(Value::Payload payload) -> Json;

	explicit Parser(std::string_view text, ParseMode mode);

	[[nodiscard]] auto parse() -> Result;

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
