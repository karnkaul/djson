#pragma once
#include <cstdint>
#include <string_view>

namespace dj::detail {
struct CharSpan {
	std::size_t first{};
	std::size_t last{};

	constexpr std::string_view view(std::string_view text) const {
		if (first >= text.size()) { return {}; }
		auto const size = last - first;
		return text.substr(first, size >= text.size() ? std::string_view::npos : size);
	}
};

struct Loc {
	CharSpan span{};
	std::size_t line_start{};
	std::uint32_t line{1};
};
} // namespace dj::detail
