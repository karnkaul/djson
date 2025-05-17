#pragma once
#include <detail/token.hpp>
#include <algorithm>
#include <array>
#include <cassert>
#include <expected>
#include <ranges>

namespace dj::detail {
struct ScanError {
	enum class Type : std::int8_t {
		UnrecognizedToken,
		MissingClosingQuote,
	};

	Type type{};
	std::string_view token{};
	SrcLoc src_loc{};
};

class Scanner {
	[[nodiscard]] static constexpr auto is_space(char const c) -> bool {
		static constexpr auto chars_v = std::array{' ', '\t', '\n', '\r'};
		return std::ranges::find(chars_v, c) != chars_v.end();
	}

	[[nodiscard]] static constexpr auto is_digit(char const c) -> bool { return c >= '0' && c <= '9'; }

	[[nodiscard]] static constexpr auto is_part_number(char const c) -> bool {
		static constexpr auto chars_v = std::array{'.', 'e', 'E', '-', '+'};
		return is_digit(c) || std::ranges::find(chars_v, c) != chars_v.end();
	}

	constexpr void advance() {
		assert(!m_remain.empty());
		if (m_remain.front() == '\n') {
			++m_src_loc.line;
			m_src_loc.column = 1;
		} else {
			++m_src_loc.column;
		}
		m_remain.remove_prefix(1);
	}

	constexpr void trim_front() {
		while (!m_remain.empty() && is_space(m_remain.front())) { advance(); }
	}

	template <typename T>
	[[nodiscard]] constexpr auto to_token(T const type, std::uint64_t const length) -> Token {
		auto src_loc = m_src_loc;
		auto const ret = Token{
			.type = type,
			.lexeme = m_remain.substr(0, length),
			.src_loc = src_loc,
		};
		if (length > 0) {
			for (std::uint64_t i = 0; i < length; ++i) { advance(); }
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
			if (!m_remain.starts_with(op_str)) { continue; }
			out = to_token(token::Operator(index), op_str.size());
			return true;
		}
		return false;
	}

	[[nodiscard]] constexpr auto try_number(Token& out) -> bool {
		if (m_remain.front() != '-' && !is_digit(m_remain.front())) { return false; }
		auto index = 1uz;
		for (; index < m_remain.size() && is_part_number(m_remain.at(index)); ++index) {}
		auto const length = index;
		out = to_token(token::Number{.raw_str = m_remain.substr(0, length)}, length);
		return true;
	}

	[[nodiscard]] constexpr auto scan_string() -> std::expected<Token, ScanError> {
		assert(m_remain.starts_with("\""));
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
			if (ch == '\"') { break; }
		}
		if (index == m_remain.size()) { return std::unexpected(to_scan_error(ScanError::Type::MissingClosingQuote, 1)); }

		auto const length = index - 1;
		return to_token(token::String{.escaped = m_remain.substr(1, length)}, length + 2);
	}

	std::string_view m_remain{};
	SrcLoc m_src_loc{};

  public:
	explicit constexpr Scanner(std::string_view const text) : m_remain(text) {
		trim_front();
		if (!m_remain.empty()) { m_src_loc = {.line = 1, .column = 1}; }
	}

	[[nodiscard]] constexpr auto next() -> std::expected<Token, ScanError> {
		if (m_remain.empty()) { return to_token(token::Eof{}, 0); }

		auto ret = Token{.src_loc = m_src_loc};

		if (try_operator(ret)) { return ret; }
		if (try_number(ret)) { return ret; }
		if (m_remain.starts_with("\"")) { return scan_string(); }

		return std::unexpected(to_scan_error(ScanError::Type::UnrecognizedToken, 1));
	}
};
} // namespace dj::detail
