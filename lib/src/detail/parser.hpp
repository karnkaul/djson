#pragma once
#include <detail/scanner.hpp>
#include <detail/value.hpp>
#include <djson/json.hpp>

namespace dj::detail {
class Parser {
  public:
	[[nodiscard]] static auto make_json(Value::Payload payload) -> Json;

	explicit Parser(std::string_view text);

	[[nodiscard]] auto parse() -> Result;

  private:
	void advance();
	void consume(token::Operator expected, Error::Type on_error);
	[[nodiscard]] auto consume_if(token::Operator expected) -> bool;

	[[nodiscard]] auto make_error(Error::Type type) const -> Error;

	[[nodiscard]] auto parse_value() -> Json;

	[[nodiscard]] auto from_operator(token::Operator op) -> Json;
	template <typename T>
	[[nodiscard]] auto make_number(token::Number in) -> Json;
	[[nodiscard]] auto make_string(token::String in) -> Json;

	[[nodiscard]] auto make_array() -> Json;
	[[nodiscard]] auto make_object() -> Json;

	[[nodiscard]] auto unescape_string(token::String in) const -> std::string;
	[[nodiscard]] auto make_key() -> std::string;

	Scanner m_scanner;
	Token m_current{};
};
} // namespace dj::detail
