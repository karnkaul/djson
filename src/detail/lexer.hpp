#pragma once
#include <deque>
#include <optional>
#include <string>
#include <vector>
#include <detail/common.hpp>

namespace dj::detail {
class lexer {
  public:
	lexer(std::string_view text) : m_text(text), m_index(0), m_loc({1, 0}) {
	}

	std::optional<token_t> advance();
	std::vector<token_t> peek(std::size_t count = 1);

	constexpr location_t const& location() const noexcept {
		return m_loc;
	}

  private:
	struct substring_t;
	struct scan_t {
		token_t token;
		location_t next_loc;
		std::size_t next_idx = 0;
	};

	scan_t next();
	std::optional<token_t> match(substring_t const& sub, location_t const& loc) const;
	std::pair<location_t, std::size_t> seek(location_t const& loc, std::size_t end) const;
	token_t make_token(std::size_t begin, std::size_t length, location_t const& loc, tk_type type) const;
	template <typename F>
	scan_t make_value(substring_t sub, std::size_t begin, location_t const& loc, F pred) const;

	std::string_view m_text;
	std::deque<scan_t> m_next;
	std::size_t m_index = 0;
	location_t m_loc;
};
} // namespace dj::detail
