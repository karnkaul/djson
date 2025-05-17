#pragma once
#include <cstdint>

namespace dj {
struct SrcLoc {
	std::uint64_t line{};
	std::uint64_t column{};
};
} // namespace dj
