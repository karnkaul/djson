#pragma once
#include <string>

namespace dj::detail {
enum class tk_type { none, curlyl, curlyr, squarel, squarer, comma, colon, etrue, efalse, eint, efloat, string, null, unknown, eof };
constexpr std::string_view tk_type_str[] = {"none",	 "{",	  "}",		 "[",		 "]",	   ",",			":",  "true",
											"false", "(int)", "(float)", "(string)", "(null)", "(unknown)", "EOF"};
struct loc_t {
	std::size_t line = 1;
	std::size_t col = 1;
};

struct lexeme_t {
	std::size_t begin{};
	std::size_t size = 1;

	std::string_view str(std::string_view full) const noexcept;
	loc_t loc(std::string_view full) const noexcept;
};

struct token_t {
	lexeme_t lexeme;
	tk_type type{};
};

class tokenizer_t {
  public:
	tokenizer_t(std::string_view text) noexcept : m_text(text) {}

	token_t scan();

  private:
	constexpr char advance() noexcept { return m_text[m_idx++]; }
	constexpr void advance(std::size_t count) noexcept { m_idx += count; }
	constexpr char peek() const noexcept { return m_idx >= m_text.size() ? '\0' : m_text[m_idx]; }
	constexpr bool expect(std::string_view str) const noexcept { return m_text.substr(m_idx, str.size()) == str; }
	constexpr std::size_t remain() const noexcept { return m_text.size() - m_idx; }
	constexpr token_t eof(std::size_t begin) const noexcept { return {{begin, 1}, tk_type::eof}; }

	void fill_token(token_t& out) const noexcept;
	void reserved(token_t& out, std::string_view str, tk_type type) const noexcept;
	void munch(token_t& out) const noexcept;
	bool escaped(std::size_t idx) const noexcept;

	std::string_view m_text;
	std::size_t m_idx{};
};
} // namespace dj::detail
