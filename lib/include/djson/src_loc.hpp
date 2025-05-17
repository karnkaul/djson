#pragma once
#include <cstdint>

namespace dj {
/// \brief Source location.
struct SrcLoc {
	std::uint64_t line{};
	std::uint64_t column{};
};
} // namespace dj
